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

./sample_hello_world_detector <image_file>

./sample_hello_world_detector <audio_file> 0 60000

./sample_hello_world_detector <video_file> 0 100 5


# REGISTER PACKAGE

As an admin user, upload HelloWorldComponent.tar.gz
to the Component Registration page. 
