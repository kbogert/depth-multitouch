#ifndef _LIBDMULTITOUCH_C
#define _LIBDMULTITOUCH_C

#include <stdint.h>
#include <vector>

using namespace std;

typedef struct touch {
    int x;
    int y;
    double radius;

} touch;


class depth_multitouch {

    public:
        depth_multitouch();
        void set_image_size(int width, int height);
        void set_height_of_touch(int units);
        void set_base(uint16_t * depthmap);

        void update(uint16_t * depthmap);

        uint8_t * get_touch_map();
        vector<struct touch> * get_touches();
        uint8_t * get_connected_regions();
        uint8_t * get_depth_image();

    private:
        void * data;
        int height;
        int width;

};


#endif

