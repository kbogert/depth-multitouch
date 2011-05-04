
#include <Python.h>

#include "libdmultitouch.c"


static void * py_d_multitouch_multitouch_data;
static int py_d_multitouch_image_width;
static int py_d_multitouch_image_height;
static uint16_t * py_d_multitouch_temp_array;

static PyObject *
dmultitouch_setimagesize(PyObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "ii", &py_d_multitouch_image_width, &py_d_multitouch_image_height))
        return NULL;

    if (py_d_multitouch_temp_array != 0) {
        free(py_d_multitouch_temp_array);
    }

    py_d_multitouch_temp_array = (uint16_t*)malloc(sizeof(uint16_t)*py_d_multitouch_image_width*py_d_multitouch_image_height);

    Py_RETURN_NONE;
}

static PyObject *
dmultitouch_setheightoftouch(PyObject *self, PyObject *args) {

    int units;
    if (!PyArg_ParseTuple(args, "i", &units))
        return NULL;

    d_multitouch_set_height_of_touch(units, py_d_multitouch_multitouch_data);

    Py_RETURN_NONE;
}


static PyObject *
dmultitouch_setbase(PyObject *self, PyObject *args)
{
    const char * data;
    int size;

    if (!PyArg_ParseTuple(args, "s#|ii", &data, &size, &py_d_multitouch_image_width, &py_d_multitouch_image_height))
        return NULL;

    int i = 0;

    for (; i < py_d_multitouch_image_width * py_d_multitouch_image_height && i < size; i++) {
        py_d_multitouch_temp_array[i] = data[i];
    }

    // parse the args as a reference to a huge list
    // convert the list to a c array

    d_multitouch_free_basemap(py_d_multitouch_multitouch_data);
    d_multitouch_set_base(py_d_multitouch_temp_array, py_d_multitouch_image_width, py_d_multitouch_image_height, py_d_multitouch_multitouch_data);

    Py_RETURN_NONE;
}

static PyObject *
dmultitouch_setbase16(PyObject *self, PyObject *args)
{
    const char * data;
    int size;

    if (!PyArg_ParseTuple(args, "s#|ii", &data, &size, &py_d_multitouch_image_width, &py_d_multitouch_image_height))
        return NULL;

    int i = 0;

    for (; i < py_d_multitouch_image_width * py_d_multitouch_image_height&& i < size /2 ; i++) {
        py_d_multitouch_temp_array[i] = (data[i * 2] << 8) + data[(i * 2) + 1];
    }

    // parse the args as a reference to a huge list
    // convert the list to a c array

    d_multitouch_free_basemap(py_d_multitouch_multitouch_data);
    d_multitouch_set_base(py_d_multitouch_temp_array, py_d_multitouch_image_width, py_d_multitouch_image_height, py_d_multitouch_multitouch_data);

    Py_RETURN_NONE;
}

static PyObject *
dmultitouch_update(PyObject *self, PyObject *args) {

    // parse the args as a reference to a huge list
    // convert the list to a c array
    const char * data;
    int size;

    if (!PyArg_ParseTuple(args, "s#", &data, &size))
        return NULL;

    int i = 0;

    for (; i < py_d_multitouch_image_width * py_d_multitouch_image_height&& i < size ; i++) {
        py_d_multitouch_temp_array[i] = data[i];
    }

    d_multitouch_update(py_d_multitouch_temp_array, py_d_multitouch_multitouch_data);
    Py_RETURN_NONE;
}


static PyObject *
dmultitouch_update16(PyObject *self, PyObject *args) {

    // parse the args as a reference to a huge list
    // convert the list to a c array
    const char * data;
    int size;

    if (!PyArg_ParseTuple(args, "s#", &data, &size))
        return NULL;

    int i = 0;

    for (; i < py_d_multitouch_image_width * py_d_multitouch_image_height&& i < size /2 ; i++) {
        py_d_multitouch_temp_array[i] = (data[i * 2] << 8) + data[(i * 2) + 1];
    }

    d_multitouch_update(py_d_multitouch_temp_array, py_d_multitouch_multitouch_data);
    Py_RETURN_NONE;
}

static PyObject *
dmultitouch_gettouches(PyObject *self, PyObject *args) {

    struct d_multitouch_touch * touches = d_multitouch_get_touches(py_d_multitouch_multitouch_data);

    PyObject *lst = PyList_New(0);
    if (!lst)
        return NULL;
    while(touches != 0) {
        PyObject *obj = PyTuple_New(3);
        if (!obj) {
            Py_DECREF(lst);
            return NULL;
        }
        PyObject *x = PyInt_FromLong(touches->x);
        if (!x) {
            Py_DECREF(lst);
            return NULL;
        }
        PyTuple_SetItem(obj, 0, x);
        PyObject *y = PyInt_FromLong(touches->y);
        if (!y) {
            Py_DECREF(lst);
            return NULL;
        }
        PyTuple_SetItem(obj, 1, y);
        PyObject *r = PyFloat_FromDouble(touches->r);
        if (!r) {
            Py_DECREF(lst);
            return NULL;
        }
        PyTuple_SetItem(obj, 2, r);

        PyList_Append(lst, obj);   // reference to num stolen
        touches = touches->next;
    }
    return lst;
}

static PyObject *
dmultitouch_gettouchmap(PyObject *self, PyObject *args) {
    return PyString_FromStringAndSize(d_multitouch_get_touch_map(py_d_multitouch_multitouch_data), py_d_multitouch_image_width * py_d_multitouch_image_height * 3);
}

static PyObject *
dmultitouch_getconnectedregions(PyObject *self, PyObject *args) {
    return PyString_FromStringAndSize(d_multitouch_get_connected_regions(py_d_multitouch_multitouch_data), py_d_multitouch_image_width * py_d_multitouch_image_height * 3);
}

static PyObject *
dmultitouch_getdepthimage(PyObject *self, PyObject *args) {
    return PyString_FromStringAndSize(d_multitouch_get_depth_filter_debug(py_d_multitouch_multitouch_data), py_d_multitouch_image_width * py_d_multitouch_image_height * 3);
}

static PyMethodDef DMultitouchMethods[] = {
    {"setImageSize",  dmultitouch_setimagesize, METH_VARARGS, "Set the size of the depth image in pixels: width and height."},
    {"setHeightOfTouch",  dmultitouch_setheightoftouch, METH_VARARGS, "Sets the height above the background to search for a touch."},
    {"setBase",  dmultitouch_setbase, METH_VARARGS, "Sets the base image, used for background subtraction, takes in a string and assumes 8 bit pixels."},
    {"setBase16",  dmultitouch_setbase16, METH_VARARGS, "Sets the base image, used for background subtraction, takes in a string and assumes 16 bit pixels."},
    {"update",  dmultitouch_update, METH_VARARGS, "Update with the current depth image, takes in a string and assumes 8 bit pixels."},
    {"update16",  dmultitouch_update16, METH_VARARGS, "Update with the current depth image, takes in a string and assumes 16 bit pixels."},
    {"getTouches",  dmultitouch_gettouches, METH_VARARGS, "Returns a list of tuples (x, y, radius) representing the current position of touches"},
    {"getTouchmap",  dmultitouch_gettouchmap, METH_VARARGS, "Returns an image with the current touches drawn on it in the blue channel.  The image is just a long string of red, green, blue values"},
    {"getConnectedRegions",  dmultitouch_getconnectedregions, METH_VARARGS, "Returns an image with the current opencv connected regions drawn on it in the blue channel.  The image is just a long string of red, green, blue values"},
    {"getDepthImage",  dmultitouch_getdepthimage, METH_VARARGS, "Returns an image with the current opencv connected regions as well as finger filter info.  The image is just a long string of red, green, blue values"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initdmultitouch(void)
{
    py_d_multitouch_multitouch_data = d_multitouch_init();
    py_d_multitouch_image_height = -1;
    py_d_multitouch_image_width = -1;
    py_d_multitouch_temp_array = 0;
    (void) Py_InitModule("dmultitouch", DMultitouchMethods);

}
