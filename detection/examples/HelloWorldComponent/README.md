# BUILD

NOTE: To build and run this component, OpenCV 4.9.0
      must be installed first.

NOTE: You must build the MPF Component API library before
      building this component. See the instructions
      in the README at the top-level component API
      directory.

The CMakeLists.txt file for this example is written to look for
OpenCV 4.9.0 installed at `/opt/opencv-4.9.0`. If your local
installation is in a different location, edit the CMakeLists.txt
file in this directory to change the `find_package` command so
that the PATHS option specifies the root directory of your
OpenCV 4.9.0 installation.

Run the following commands:
```
mkdir build
cd build
cmake3 ..
make install
```
The built plugin package will be in `build/plugin-packages`


# RUN STANDALONE

From within the build directory:

./sample_hello_world_detector

./sample_hello_world_detector -i <image_file>

./sample_hello_world_detector -a <audio_file> 0 60000

./sample_hello_world_detector -v <video_file> 0 100

./sample_hello_world_detector -g <generic_file>


# REGISTER PACKAGE

As an admin user, upload HelloWorldComponent.tar.gz
to the Component Registration page.
