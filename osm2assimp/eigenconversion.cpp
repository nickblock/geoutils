#include <iostream>
#include "eigenconversion.h"

constexpr double PI = 3.14159265358979323846;
constexpr double EARTH_DIAMETERS_METERS = 6371.0 * 2 * 1000;
constexpr double DEGREES_TO_RADIANS = PI / 180.0;
constexpr double RADIANS_TO_DEGREES = 180.0 / PI;

inline auto degreeToRadian(const double degree) -> double {
    return (degree * PI / 180);
};

inline auto radianToDegree(const double radian) -> double {
    return (radian * 180 / PI);
};

inline auto ECEFrotation(const Eigen::Vector3d &origin) -> Eigen::Matrix3d {
    double lat = origin[0] * DEGREES_TO_RADIANS;
    double lon = origin[1] * DEGREES_TO_RADIANS;
    Eigen::Matrix3d R;
    R << -sin(lat) * cos(lon), -sin(lat) * sin(lon), cos(lat),
    -sin(lon), cos(lon), 0,
    -cos(lat) * cos(lon), -cos(lat) * sin(lon), -sin(lat);
    return R;
}

auto LLAtoNED(const Eigen::Vector3d &origin, const CoordinateArray &lla, CoordinateArray &ned) -> void {
    CoordinateArray ecef;
    LLAtoECEF(lla, ecef);
    ECEFtoNED(origin, ecef, ned);
}

auto LLAtoNED(const Eigen::Vector3d &origin, const Eigen::Vector3d& lla) -> Eigen::Vector3d {
    const Eigen::Vector3d ecef = LLAtoECEF(lla);
    return ECEFtoNED(origin, ecef);
}

auto NEDtoLLA(const Eigen::Vector3d &origin, const CoordinateArray &ned, CoordinateArray &lla) -> void {
    CoordinateArray ecef;
    NEDtoECEF(origin, ned, ecef);
    ECEFtoLLA(ecef, lla);
}

auto NEDtoLLA(const Eigen::Vector3d& origin, const Eigen::Vector3d& ned) -> Eigen::Vector3d {
    const Eigen::Vector3d ecef = NEDtoECEF(origin, ned);
    return ECEFtoLLA(ecef);
}

auto LLAtoECEF(const CoordinateArray &lla, CoordinateArray &ecef) -> void {
    const Eigen::Array<double, 1, Eigen::Dynamic> lat = lla.row(0) * DEGREES_TO_RADIANS;
    const Eigen::Array<double, 1, Eigen::Dynamic> lon = lla.row(1) * DEGREES_TO_RADIANS;
    const Eigen::Array<double, 1, Eigen::Dynamic> alt = lla.row(2);
    const Eigen::Array<double, 1, Eigen::Dynamic> xiInv =
    (1 - WGS84::FIRST_ECCENTRICITY_SQR * lat.sin() * lat.sin()).rsqrt();
    ecef.resize(Eigen::NoChange, lla.cols());
    ecef.row(0) = (WGS84::SEMI_MAJOR_AXIS * xiInv + alt) * lat.cos() * lon.cos();
    ecef.row(1) = (WGS84::SEMI_MAJOR_AXIS * xiInv + alt) * lat.cos() * lon.sin();
    ecef.row(2) = (WGS84::SEMI_MAJOR_AXIS * xiInv * (1 - WGS84::FIRST_ECCENTRICITY_SQR) + alt) * lat.sin();
}

auto LLAtoECEF(const Eigen::Vector3d& lla) -> Eigen::Vector3d {
    CoordinateArray llaArray(lla);
    CoordinateArray ecef;
    LLAtoECEF(lla, ecef);
    return ecef.col(0);
}

auto ECEFtoLLA(const CoordinateArray &ecef, CoordinateArray &lla) -> void {
    const Eigen::Array<double, 1, Eigen::Dynamic> x = ecef.row(0);
    const Eigen::Array<double, 1, Eigen::Dynamic> y = ecef.row(1);
    const Eigen::Array<double, 1, Eigen::Dynamic> z = ecef.row(2);
    const Eigen::Array<double, 1, Eigen::Dynamic> r = (x.square() + y.square()).sqrt();
    
    constexpr double esq = WGS84::SEMI_MAJOR_AXIS * WGS84::SEMI_MAJOR_AXIS - WGS84::SEMI_MINOR_AXIS * WGS84::SEMI_MINOR_AXIS;
    const Eigen::Array<double, 1, Eigen::Dynamic> F = 54 * WGS84::SEMI_MINOR_AXIS * WGS84::SEMI_MINOR_AXIS * z.square();
    const Eigen::Array<double, 1, Eigen::Dynamic> G =
    r.square() + (1 - WGS84::FIRST_ECCENTRICITY_SQR) * z.square() - WGS84::FIRST_ECCENTRICITY_SQR * esq;
    const Eigen::Array<double, 1, Eigen::Dynamic> C =
    (WGS84::FIRST_ECCENTRICITY_SQR * WGS84::FIRST_ECCENTRICITY_SQR * F * r.square()) / (G.cube());
    const Eigen::Array<double, 1, Eigen::Dynamic> S = (1 + C + (C.square() + 2 * C).sqrt()).pow(1.0/3.0);
    const Eigen::Array<double, 1, Eigen::Dynamic> P = F / (3 * (S + S.inverse() + 1).square() * G.square());
    const Eigen::Array<double, 1, Eigen::Dynamic> Q  =
    (1 + 2 * WGS84::FIRST_ECCENTRICITY_SQR * WGS84::FIRST_ECCENTRICITY_SQR * P).sqrt();
    const Eigen::Array<double, 1, Eigen::Dynamic> r0 =
    -(P * WGS84::FIRST_ECCENTRICITY_SQR * r) / (1 + Q)
    + (0.5 * WGS84::SEMI_MAJOR_AXIS * WGS84::SEMI_MAJOR_AXIS * (1 + Q.inverse()) -
       P * (1 - WGS84::FIRST_ECCENTRICITY_SQR) * z.square() / (Q * (1 + Q)) - 0.5 * P * r.square()).sqrt();
    const Eigen::Array<double, 1, Eigen::Dynamic> U =
    ((r - WGS84::FIRST_ECCENTRICITY_SQR * r0).square() + z.square()).sqrt();
    const Eigen::Array<double, 1, Eigen::Dynamic> V =
    ((r - WGS84::FIRST_ECCENTRICITY_SQR * r0).square() + (1 - WGS84::FIRST_ECCENTRICITY_SQR) * z.square()).sqrt();
    const Eigen::Array<double, 1, Eigen::Dynamic> Z0 =
    WGS84::SEMI_MINOR_AXIS * WGS84::SEMI_MINOR_AXIS * z / (WGS84::SEMI_MAJOR_AXIS * V);
    lla.resize(Eigen::NoChange, ecef.cols());
    lla.row(0) = ((z + WGS84::SECOND_ECCENTRICITY_SQR * Z0) / r).atan() * RADIANS_TO_DEGREES;
    lla.row(1) = y.binaryExpr(x, [] (double a, double b) { return std::atan2(a, b); }) * RADIANS_TO_DEGREES;
    lla.row(2) = U * (1 - WGS84::SEMI_MINOR_AXIS * WGS84::SEMI_MINOR_AXIS  / (WGS84::SEMI_MAJOR_AXIS * V));
}

auto ECEFtoLLA(const Eigen::Vector3d& ecef) -> Eigen::Vector3d {
    CoordinateArray ecefArray(ecef);
    CoordinateArray lla;
    ECEFtoLLA(ecef, lla);
    return lla.col(0);
}

auto ECEFtoNED(const Eigen::Vector3d &origin, const CoordinateArray &ecef, CoordinateArray &ned) -> void {
    const Eigen::Vector3d originECEF = LLAtoECEF(origin);
    const auto R = ECEFrotation(origin);
    
    ned = R * (ecef.colwise() - originECEF);
}

auto ECEFtoNED(const Eigen::Vector3d &origin, const Eigen::Vector3d &ecef) -> Eigen::Vector3d {
    const auto originECEF = LLAtoECEF(origin);
    const auto R = ECEFrotation(origin);
    return R * (ecef - originECEF);
}

auto NEDtoECEF(const Eigen::Vector3d &origin, const CoordinateArray &ned, CoordinateArray &ecef) -> void {
    const auto R = ECEFrotation(origin);
    ecef = R.transpose() * ned;
    ecef.colwise() += LLAtoECEF(origin);
}

auto NEDtoECEF(const Eigen::Vector3d &origin, const Eigen::Vector3d &ned) -> Eigen::Vector3d {
    const auto R = ECEFrotation(origin);
    return LLAtoECEF(origin) + R.transpose() * ned;
}

auto angleBetweenCoordinates(double lat1, const double long1, double lat2, const double long2) -> double {
    const auto longitudeDifference = degreeToRadian(long2 - long1);
    lat1 = degreeToRadian(lat1);
    lat2 = degreeToRadian(lat2);
    
    using namespace std;
    const auto x = (cos(lat1) * sin(lat2)) - (sin(lat1) * cos(lat2) * cos(longitudeDifference));
    const auto y = sin(longitudeDifference) * cos(lat2);
    
    const auto degree = radianToDegree(atan2(y, x));
    return (degree >= 0)? degree : (degree + 360);
}
    
auto metersBetweenCoordinates(double lat1, double long1, double lat2, double long2) -> double {
    lat1 = degreeToRadian(lat1);
    long1 = degreeToRadian(long1);
    lat2 = degreeToRadian(lat2);
    long2 = degreeToRadian(long2);
    
    using namespace std;
    double x = sin((lat2 - lat1) / 2), y = sin((long2 - long1) / 2);
#if 1
    return EARTH_DIAMETERS_METERS * asin(sqrt((x * x) + (cos(lat1) * cos(lat2) * y * y)));
#else
    double value = (x * x) + (cos(lat1) * cos(lat2) * y * y);
    return EARTH_DIAMETERS_METERS * atan2(sqrt(value), sqrt(1 - value));
#endif
}