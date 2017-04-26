# Overview

This repository contains source code for the Open Media Processing Framework (OpenMPF) C++ Component SDK, 
including the API and associated utilities.


# Build and Install
* If not installed, install [CMake](https://cmake.org/) version 3.6 or higher.
* Then, cd into the `openmpf-cpp-component-sdk` directory.
* Then, run the following commands to build and install the SDK:
```
mkdir build
cd build
cmake3 ..
make install
```

### Changing the Install Location
By default the SDK will get installed to ~/mpf-sdk-install. 
To change the install location you can set the MPF_SDK_INSTALL_PATH environment variable to a different directory.

