# BUILD

NOTE: To build and run this component, OpenCV 4.5.0
      must be installed first.

NOTE: You must build the MPF Component API library before
      building this component. See the instructions
      in the README at the top-level component API
      directory.

Before you build, edit the src/CMakeLists.txt file, and change
the "find_package" command so that the PATHS option specifies
the root directory of your OpenCV 4.5.0 installation.

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

./sample_audio_only_detector

./sample_audio_only_detector <audio_file> 0 60000


# REGISTER PACKAGE

As an admin user, upload AudioOnlyComponent.tar.gz
to the Component Registration page.
