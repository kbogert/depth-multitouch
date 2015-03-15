# Bindings #

## C/C++: ##

  1. You will need opencv installed on your system
  1. For c, just add libdmultitouch.c to your project
  1. For c++, add libdmultitouch.hpp and libdmultitouch.cpp to your project
    * Then place libdmultitouch.c somewhere in your include path

## Python: ##

  1. You will need opencv installed on your system (though not necessarily the python bindings for it)
  1. Type: python setup.py build
  1. Type: python setup.py install
  1. Once installed, you can use it just by: import dmultitouch

# Quick Demo #

  1. You will need openkinect, opengl, opencv, and vrpn installed on your system
  1. Compile kinectdemo.cpp (Code::Blocks project provided)
  1. Hook up your kinect and point it at a wall
  1. Run the program
  1. Move yourself out if its view and press 'p'
  1. Now touch the wall