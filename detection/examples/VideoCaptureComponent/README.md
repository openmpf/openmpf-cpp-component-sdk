# BUILD

NOTE: To build and run this component, OpenCV 3.1.0 and
      ffmpeg must be installed first.

NOTE: You must build the MPF Component API library before
      building this component. See the instructions
      in the README at the top-level component API
      directory.

Before you build, edit the src/CMakeLists.txt file, and change
the "find_package" command so that the PATHS option specifies
the root directory of your OpenCV 3.1.0 installation.

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

./sample_video_capture_component

./sample_video_capture_component <video_file> 0 20  ROTATE

./sample_video_capture_component <video_file> 0 20 FLIP

./sample_video_capture_component <video_file> 0 20 CROP


# REGISTER PACKAGE

As an admin user, upload VideoCaptureComponent.tar.gz
to the Component Registration page. 
