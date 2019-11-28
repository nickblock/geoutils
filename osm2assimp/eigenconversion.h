#pragma once

#include <Eigen/Geometry>

namespace WGS84 {
  constexpr double SEMI_MAJOR_AXIS = 6378137;
  constexpr double FLATTENING_FACTOR = 1 / 298.257223563;
  constexpr double SEMI_MINOR_AXIS = SEMI_MAJOR_AXIS * (1 - FLATTENING_FACTOR);
  constexpr double FIRST_ECCENTRICITY_SQR = 2 * FLATTENING_FACTOR - FLATTENING_FACTOR * FLATTENING_FACTOR;
  constexpr double SECOND_ECCENTRICITY_SQR =
      FLATTENING_FACTOR * (2 - FLATTENING_FACTOR) / ((1 - FLATTENING_FACTOR) * (1 - FLATTENING_FACTOR));
}

using CoordinateArray = Eigen::Matrix<double, 3, Eigen::Dynamic>;

auto LLAtoNED(const Eigen::Vector3d& origin, const CoordinateArray& lla, CoordinateArray& ned) -> void;
auto LLAtoNED(const Eigen::Vector3d& origin, const Eigen::Vector3d& lla) -> Eigen::Vector3d;
auto NEDtoLLA(const Eigen::Vector3d& origin, const CoordinateArray& ned, CoordinateArray& lla) -> void;
auto NEDtoLLA(const Eigen::Vector3d& origin, const Eigen::Vector3d& ned) -> Eigen::Vector3d;

auto LLAtoECEF(const CoordinateArray& lla, CoordinateArray& ecef) -> void;
auto LLAtoECEF(const Eigen::Vector3d& lla) -> Eigen::Vector3d;
auto ECEFtoLLA(const CoordinateArray& ecef, CoordinateArray& lla) -> void;
auto ECEFtoLLA(const Eigen::Vector3d& ecef) -> Eigen::Vector3d;

auto ECEFtoNED(const Eigen::Vector3d& origin, const CoordinateArray& ecef, CoordinateArray& ned) -> void;
auto ECEFtoNED(const Eigen::Vector3d& origin, const Eigen::Vector3d& ecef) -> Eigen::Vector3d;
auto NEDtoECEF(const Eigen::Vector3d& origin, const CoordinateArray& ned, CoordinateArray& ecef) -> void;
auto NEDtoECEF(const Eigen::Vector3d& origin, const Eigen::Vector3d& ned) -> Eigen::Vector3d;

auto angleBetweenCoordinates(double lat1, const double long1, double lat2, const double long2) -> double;
auto metersBetweenCoordinates(double lat1, double long1, double lat2, double long2) -> double;    

