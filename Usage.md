# Introduction #

The use of the library is pretty simple:
  1. Initialize
  1. Set the:
    * Size of the depth image
    * maximum height off the background to detect a touch
  1. Set the base image
  1. repeated call update() with the new depthmaps
  1. call the getTouches() function to retrieve the current touches


# Details #

## C++ ##

  * #include "libdmultitouch.hpp"
  * depth\_multitouch multitouch\_lib;
  * multitouch\_lib.set\_image\_size(640, 480);
  * multitouch\_lib.set\_height\_of\_touch(7);
  * multitouch\_lib.set\_base(array);
    * array must be an array of uint16\_t, of size width `*` height
  * multitouch\_lib.update(array);
    * array must be an array of uint16\_t, of size width `*` height
  * multitouch\_lib.get\_touches();
    * returns a vector of: struct touch entries for each touch

## C ##

  * #include "libdmultitouch.c"
  * void `*` data = d\_multitouch\_init();
  * d\_multitouch\_set\_height\_of\_touch(7, data);
  * d\_multitouch\_free\_basemap(data);
  * d\_multitouch\_set\_base(depthmap, 640, 480, data);
  * d\_multitouch\_update(depthmap, data);
  * struct d\_multitouch\_touch `*` touches = d\_multitouch\_get\_touches(data)
## Python ##


  * import dmultitouch
  * dmultitouch.setHeightOfTouch(7)
  * dmultitouch.setImageSize(640,480)
  * dmultitouch.setBase(d)
    * d must be either a string or bytes, and is interpreted as a one-dimensional row-major unroll of the 2d image, with each pixel being one byte, of size width `*` height
  * dmultitouch.setBase16(d)
    * d must be either a string or bytes, and is interpreted as a one-dimensional row-major unroll of the 2d image, with each pixel being two bytes, of size width `*` height `*` 2
  * dmultitouch.update(d)
    * d must be either a string or bytes, and is interpreted as a one-dimensional row-major unroll of the 2d image, with each pixel being one byte, of size width `*` height
  * dmultitouch.update16(d)
    * d must be either a string or bytes, and is interpreted as a one-dimensional row-major unroll of the 2d image, with each pixel being two bytes, of size width `*` height `*` 2
  * dmultitouch.getTouches()
    * returns a list of tuples, where each tuple contains (x, y, radius) of a single touch