# BUILD

WARNING: This example is not compatible with OpenCV 3.1.0.
         Functionality was introduced in that release so
         that when a jpeg file with EXIF information is read by
         cv::imread(), it automatically uses the EXIF information to
         return a transformed image, but there have been two problems
         reported with this. The first problem is that the new code
         may hang when reading certain jpeg files
         (https://github.com/opencv/opencv/issues?utf8=✓&q=6641). The
         second problem is that you cannot tell imread() to ignore the
         EXIF data and return an untransformed image
         (https://github.com/opencv/opencv/issues?utf8=✓&q=6348). For
         these reasons, we have disabled the use of the
         MPFImageReader, and this example will not compile. To process
         MPFImageJobs, you may use the MPFVideoCapture object instead.
         The MPFImageReader will be re-enabled when the MPF Component
         API functions have been ported to OpenCV 3.2.0, where these
         problems have been fixed.

NOTE: You must build the MPF Component API library before
      building this component. See the instructions
      in the README in the "MPFComponentAPI/cplusplus"
      directory.

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
