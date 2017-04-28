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

./sample_audio_only_detector

./sample_audio_only_detector <audio_file> 0 60000


# REGISTER PACKAGE

As an admin user, upload AudioOnlyComponent.tar.gz
to the Component Registration page.
