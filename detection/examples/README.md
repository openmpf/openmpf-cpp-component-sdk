# Overview

This directory contains source code for the Open Media Processing Framework (OpenMPF) component examples.


# Building the Example Components
* In order to build the example components, the OpenMPF C++ Component SDK must be installed. 
  See `openmpf-cpp-component-sdk/README.md` for details.
* cd into the `openmpf-cpp-component-sdk/detection/examples` directory.
* Run the following commands:
```
mkdir build
cd build
cmake3 ..
make install
```
* The built plugin packages will be created in `openmpf-cpp-component-sdk/detection/examples/build/plugin-packages`.

### Building Individual Components
If you would like to only build a single component, you can cd into that component's directory and run the
build commands listed above.
