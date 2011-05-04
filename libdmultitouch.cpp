
#include "libdmultitouch.hpp"
#include "libdmultitouch.c"

depth_multitouch::depth_multitouch() {
    data = d_multitouch_init();
    height = 0;
    width = 0;
}

void depth_multitouch::set_image_size(int w, int h) {
    width = w;
    height = h;
}

void depth_multitouch::set_base(uint16_t * depthmap) {
    d_multitouch_free_basemap(data);
    d_multitouch_set_base(depthmap, width, height, data);
}

void depth_multitouch::set_height_of_touch(int units) {
    d_multitouch_set_height_of_touch(units, data);
}

void depth_multitouch::update(uint16_t * depthmap) {
    d_multitouch_update(depthmap, data);
}

uint8_t * depth_multitouch::get_touch_map() {
    return d_multitouch_get_touch_map(data);
}

vector<struct touch> * depth_multitouch::get_touches() {
    struct d_multitouch_touch * touches = d_multitouch_get_touches(data);
    std::vector<struct touch> * returnval = new std::vector<struct touch>();

    while (touches != NULL) {
        struct touch t;
        t.radius = touches->r;
        t.x = touches->x;
        t.y = touches->y;
        returnval->push_back(t);
        touches = touches->next;
    }

    return returnval;
}

uint8_t * depth_multitouch::get_connected_regions() {
    return d_multitouch_get_connected_regions(data);
}


uint8_t * depth_multitouch::get_depth_image() {
    return d_multitouch_get_depth_filter_debug(data);
}
