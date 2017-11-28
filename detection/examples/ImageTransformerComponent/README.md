# BUILD

NOTE: To build and run this component, OpenCV 3.3.0
      must be installed first.

NOTE: You must build the MPF Component API library before
      building this component. See the instructions
      in the README at the top-level component API
      directory.

Before you build, edit the src/CMakeLists.txt file, and change
the "find_package" command so that the PATHS option specifies
the root directory of your OpenCV 3.3.0 installation.

Run the following commands:
```
mkdir build
cd build
cmake3 ..
make install
```
The built plugin package will be in `build/plugin-packages`


# RUN STANDALONE

To run this component, you must supply the path to
a video file on the command line.

From within the build directory:

./sample_image_transformer

./sample_image_transformer <image_file> ROTATE

./sample_image_transformer <image_file> CROP

./sample_image_transformer <image_file> FLIP


# REGISTER PACKAGE

As an admin user, upload ImageTransformerComponent.tar.gz
to the Component Registration page. 
