# Openssl

We require this lib for both delivering curl requests as well as linking the s2 library.

The library was compiled for each platform from this source:

```
github.com:JCMais/curl-for-windows.git
```

### Build For Mac

Use gyp to create xcode project and build just the static library there.

````
cd curl-for-windows/openssl

${SCAPEKIT_ROOT}/deps/gyp/gyp --depth=. -f xcode --generator-output ${SCAPEKIT_ROOT}scapekit/deps/openssl/mac -Iopenssl_common.gypi -Dlibrary=static_library -DOS=mac -Dtarget_arch=x64
````

### Build For Windows

Follow instructions at root if "curl-for-windows" to create MSVC solution

````python configure.py --toolchain=msvc --target_arch=x64````

