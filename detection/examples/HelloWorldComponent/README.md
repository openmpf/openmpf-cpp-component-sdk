# BUILD

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

./sample_hello_world_detector -v <video_file> 0 100 5

./sample_hello_world_detector -g <generic_file>

# REGISTER PACKAGE

As an admin user, upload HelloWorldComponent.tar.gz
to the Component Registration page. 
