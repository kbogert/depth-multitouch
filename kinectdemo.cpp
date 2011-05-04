/*
 * This file is part of the OpenKinect Project. http://www.openkinect.org
 *
 * Copyright (c) 2010 individual OpenKinect contributors. See the CONTRIB file
 * for details.
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */


#include "libfreenect.hpp"
#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <cmath>
#include <vector>
#include "libdmultitouch.hpp"
#include <algorithm>

#if defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "vrpn/vrpn_Analog.h"
#include "vrpn/vrpn_Connection.h"


class Mutex {
public:
    Mutex() {
        pthread_mutex_init( &m_mutex, NULL );
    }
    void lock() {
        pthread_mutex_lock( &m_mutex );
    }
    void unlock() {
        pthread_mutex_unlock( &m_mutex );
    }
private:
    pthread_mutex_t m_mutex;
};

/* thanks to Yoda---- from IRC */
class MyFreenectDevice : public Freenect::FreenectDevice {
public:
    MyFreenectDevice(freenect_context *_ctx, int _index)
        : Freenect::FreenectDevice(_ctx, _index), m_buffer_depth(FREENECT_VIDEO_RGB_SIZE),m_buffer_video(FREENECT_VIDEO_RGB_SIZE),m_buffer_connected_regions(FREENECT_VIDEO_RGB_SIZE),m_buffer_filter_debug(FREENECT_VIDEO_RGB_SIZE),m_buffer_touches(FREENECT_VIDEO_RGB_SIZE), m_gamma(2048), m_new_rgb_frame(false), m_new_depth_frame(false) {
        set_next_frame_to_base = false;
        showTouchMap = false;
        for( unsigned int i = 0 ; i < 2048 ; i++) {
            float v = i/2048.0;
            v = std::pow(v, 3)* 6;
            m_gamma[i] = v*6*256;
        }
        multitouch_lib.set_image_size(640, 480);
        multitouch_lib.set_height_of_touch(7);
        connection = vrpn_create_server_connection();
        vrpn_server = new vrpn_Analog_Server("multitouch", connection);
        vrpn_server->setNumChannels (128);
    }
    //~MyFreenectDevice(){}
    // Do not call directly even in child
    void VideoCallback(void* _rgb, uint32_t timestamp) {
        m_rgb_mutex.lock();
        uint8_t* rgb = static_cast<uint8_t*>(_rgb);
        std::copy(rgb, rgb+getVideoBufferSize(), m_buffer_video.begin());
        m_new_rgb_frame = true;
        m_rgb_mutex.unlock();
    };
    // Do not call directly even in child
    void DepthCallback(void* _depth, uint32_t timestamp) {
        m_depth_mutex.lock();
        uint16_t* depth = static_cast<uint16_t*>(_depth);

        if (set_next_frame_to_base) {

//            d_multitouch_free_basemap(data);

            uint16_t * tempArray = (uint16_t *) malloc(sizeof(uint16_t) * 640 * 480);

            std::copy(depth, depth + FREENECT_FRAME_PIX, tempArray);

            multitouch_lib.set_base(tempArray);
//            d_multitouch_set_base(tempArray, 640, 480, data);

            free(tempArray);
            set_next_frame_to_base = false;
            showTouchMap = true;
        }

        multitouch_lib.update(depth);
//        d_multitouch_update(depth, data);

        for( unsigned int i = 0 ; i < FREENECT_FRAME_PIX ; i++) {
            int pval = m_gamma[depth[i]];
            int lb = pval & 0xff;
            switch (pval>>8) {
            case 0:
                m_buffer_depth[3*i+0] = 255;
                m_buffer_depth[3*i+1] = 255-lb;
                m_buffer_depth[3*i+2] = 255-lb;
                break;
            case 1:
                m_buffer_depth[3*i+0] = 255;
                m_buffer_depth[3*i+1] = lb;
                m_buffer_depth[3*i+2] = 0;
                break;
            case 2:
                m_buffer_depth[3*i+0] = 255-lb;
                m_buffer_depth[3*i+1] = 255;
                m_buffer_depth[3*i+2] = 0;
                break;
            case 3:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 255;
                m_buffer_depth[3*i+2] = lb;
                break;
            case 4:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 255-lb;
                m_buffer_depth[3*i+2] = 255;
                break;
            case 5:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 0;
                m_buffer_depth[3*i+2] = 255-lb;
                break;
            default:
                m_buffer_depth[3*i+0] = 0;
                m_buffer_depth[3*i+1] = 0;
                m_buffer_depth[3*i+2] = 0;
                break;
            }
        }

        if (showTouchMap) {
//            uint8_t * tempBuffer = d_multitouch_get_connected_regions(data);
            uint8_t * tempBuffer = multitouch_lib.get_connected_regions();
            m_buffer_connected_regions.assign(tempBuffer, tempBuffer + (FREENECT_FRAME_PIX * 3));
//            tempBuffer = d_multitouch_get_depth_filter_debug(data);
            tempBuffer = multitouch_lib.get_depth_image();
            m_buffer_filter_debug.assign(tempBuffer, tempBuffer + (FREENECT_FRAME_PIX * 3));
//            tempBuffer = d_multitouch_get_touch_map(data);
            tempBuffer = multitouch_lib.get_touch_map();
            m_buffer_touches.assign(tempBuffer, tempBuffer + (FREENECT_FRAME_PIX * 3));

            vrpn_float64* channels = vrpn_server->channels();

            vector<struct touch> * touches = multitouch_lib.get_touches();
            int channelNum = 0;

            for (int i = 0; i < touches->size() && channelNum < 126; i ++) {
                channels[channelNum] = (double) (*touches)[i].x;
                channelNum ++;
                channels[channelNum] = (double) (*touches)[i].y;
                channelNum ++;
                channels[channelNum] = (*touches)[i].radius;
                channelNum ++;
            }

            while (channelNum < 128) {
                channels[channelNum] = -1.0;
                channelNum ++;
            }

            vrpn_server->report();
        }

        m_new_depth_frame = true;
        m_depth_mutex.unlock();
        vrpn_server->mainloop();
        connection->mainloop();
    }
    bool getRGB(std::vector<uint8_t> &buffer) {
        m_rgb_mutex.lock();
        if(m_new_rgb_frame) {
            buffer.swap(m_buffer_video);
            m_new_rgb_frame = false;
            m_rgb_mutex.unlock();
            return true;
        } else {
            m_rgb_mutex.unlock();
            return false;
        }
    }

    bool getDepth(std::vector<uint8_t> &bufferDepth, std::vector<uint8_t> &bufferConnected, std::vector<uint8_t> &bufferConnectedDebug, std::vector<uint8_t> &bufferTouches) {
        m_depth_mutex.lock();
        if(m_new_depth_frame) {

            bufferDepth.swap(m_buffer_depth);
            bufferConnected.swap(m_buffer_connected_regions);
            bufferConnectedDebug.swap(m_buffer_filter_debug);
            bufferTouches.swap(m_buffer_touches);

            m_new_depth_frame = false;
            m_depth_mutex.unlock();
            return true;
        } else {
            m_depth_mutex.unlock();
            return false;
        }
    }

    void setNextFrameToBase() {
        m_depth_mutex.lock();
        set_next_frame_to_base = true;
        m_depth_mutex.unlock();

    }
/*
    void setMultitouchData(void * mdata) {
        data = mdata;
    }
*/
private:
    std::vector<uint8_t> m_buffer_depth;
    std::vector<uint8_t> m_buffer_connected_regions;
    std::vector<uint8_t> m_buffer_filter_debug;
    std::vector<uint8_t> m_buffer_touches;

    std::vector<uint8_t> m_buffer_video;
    std::vector<uint16_t> m_gamma;
    Mutex m_rgb_mutex;
    Mutex m_depth_mutex;
    bool m_new_rgb_frame;
    bool m_new_depth_frame;
    bool set_next_frame_to_base;
    bool showTouchMap;
    depth_multitouch multitouch_lib;
    vrpn_Analog_Server * vrpn_server;
    vrpn_Connection * connection;
};

Freenect::Freenect freenect;
MyFreenectDevice* device;
freenect_video_format requested_format(FREENECT_VIDEO_RGB);

GLuint gl_depth_tex;
GLuint gl_connected_tex;
GLuint gl_connected_debug_tex;
GLuint gl_touches_tex;


double freenect_angle(0);
int got_frames(0),window(0);
int g_argc;
char **g_argv;

void DrawGLScene() {
    static std::vector<uint8_t> depth(640*480*4);
    static std::vector<uint8_t> connected(640*480*4);
    static std::vector<uint8_t> connectedDebug(640*480*4);
    static std::vector<uint8_t> touches(640*480*4);
//    static std::vector<uint8_t> rgb(640*480*4);

    // using getTiltDegs() in a closed loop is unstable
    /*if(device->getState().m_code == TILT_STATUS_STOPPED){
    	freenect_angle = device->getState().getTiltDegs();
    }*/
    device->updateState();
    printf("\r demanded tilt angle: %+4.2f device tilt angle: %+4.2f", freenect_angle, device->getState().getTiltDegs());
    fflush(stdout);

    device->getDepth(depth, connected, connectedDebug, touches);
//    device->getRGB(rgb);

    got_frames = 0;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, &depth[0]);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0, 0);
    glVertex3f(0,0,0);
    glTexCoord2f(1, 0);
    glVertex3f(640,0,0);
    glTexCoord2f(1, 1);
    glVertex3f(640,480,0);
    glTexCoord2f(0, 1);
    glVertex3f(0,480,0);
    glEnd();


    glBindTexture(GL_TEXTURE_2D, gl_connected_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, &connected[0]);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0, 0);
    glVertex3f(640,0,0);
    glTexCoord2f(1, 0);
    glVertex3f(1280,0,0);
    glTexCoord2f(1, 1);
    glVertex3f(1280,480,0);
    glTexCoord2f(0, 1);
    glVertex3f(640,480,0);
    glEnd();


    glBindTexture(GL_TEXTURE_2D, gl_connected_debug_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, &connectedDebug[0]);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0, 0);
    glVertex3f(0,480,0);
    glTexCoord2f(1, 0);
    glVertex3f(640,480,0);
    glTexCoord2f(1, 1);
    glVertex3f(640,960,0);
    glTexCoord2f(0, 1);
    glVertex3f(0,960,0);
    glEnd();


    glBindTexture(GL_TEXTURE_2D, gl_touches_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, &touches[0]);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0, 0);
    glVertex3f(640,480,0);
    glTexCoord2f(1, 0);
    glVertex3f(1280,480,0);
    glTexCoord2f(1, 1);
    glVertex3f(1280,960,0);
    glTexCoord2f(0, 1);
    glVertex3f(640,960,0);
    glEnd();
/*
    glBindTexture(GL_TEXTURE_2D, gl_rgb_tex);
    if (device->getVideoFormat() == FREENECT_VIDEO_RGB || device->getVideoFormat() == FREENECT_VIDEO_YUV_RGB)
        glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, &rgb[0]);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, 1, 640, 480, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &rgb[0]);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0, 0);
    glVertex3f(640,0,0);
    glTexCoord2f(1, 0);
    glVertex3f(1280,0,0);
    glTexCoord2f(1, 1);
    glVertex3f(1280,480,0);
    glTexCoord2f(0, 1);
    glVertex3f(640,480,0);
    glEnd();
*/

    glutSwapBuffers();
}

void keyPressed(unsigned char key, int x, int y) {
    if (key == 27) {
        glutDestroyWindow(window);
    }
    if (key == '1') {
        device->setLed(LED_GREEN);
    }
    if (key == '2') {
        device->setLed(LED_RED);
    }
    if (key == '3') {
        device->setLed(LED_YELLOW);
    }
    if (key == '4') {
        device->setLed(LED_BLINK_GREEN);
    }
    if (key == '5') {
        // 5 is the same as 4
        device->setLed(LED_BLINK_GREEN);
    }
    if (key == '6') {
        device->setLed(LED_BLINK_RED_YELLOW);
    }
    if (key == '0') {
        device->setLed(LED_OFF);
    }
    if (key == 'f') {
        if (requested_format == FREENECT_VIDEO_IR_8BIT)
            requested_format = FREENECT_VIDEO_RGB;
        else if (requested_format == FREENECT_VIDEO_RGB)
            requested_format = FREENECT_VIDEO_YUV_RGB;
        else
            requested_format = FREENECT_VIDEO_IR_8BIT;
        device->setVideoFormat(requested_format);
    }

    if (key == 'w') {
        freenect_angle++;
        if (freenect_angle > 30) {
            freenect_angle = 30;
        }
    }
    if (key == 's' || key == 'd') {
        freenect_angle = 0;
    }
    if (key == 'x') {
        freenect_angle--;
        if (freenect_angle < -30) {
            freenect_angle = -30;
        }
    }
    if (key == 'e') {
        freenect_angle = 10;
    }
    if (key == 'c') {
        freenect_angle = -10;
    }
    if (key == 'p') {
        // set the current depth map as the multitouch's base
        device->setNextFrameToBase();
    }
    device->setTiltDegrees(freenect_angle);
}

void ReSizeGLScene(int Width, int Height) {
    glViewport(0,0,Width,Height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho (0, 1280, 960, 0, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
}

void InitGL(int Width, int Height) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
    glGenTextures(1, &gl_depth_tex);
    glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &gl_connected_tex);
    glBindTexture(GL_TEXTURE_2D, gl_connected_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &gl_connected_debug_tex);
    glBindTexture(GL_TEXTURE_2D, gl_connected_debug_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &gl_touches_tex);
    glBindTexture(GL_TEXTURE_2D, gl_touches_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ReSizeGLScene(Width, Height);
}

void *gl_threadfunc(void *arg) {
    printf("GL thread\n");

    glutInit(&g_argc, g_argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
    glutInitWindowSize(1280, 960);
    glutInitWindowPosition(0, 0);

    window = glutCreateWindow("LibFreenect");

    glutDisplayFunc(&DrawGLScene);
    glutIdleFunc(&DrawGLScene);
    glutReshapeFunc(&ReSizeGLScene);
    glutKeyboardFunc(&keyPressed);

    InitGL(1280, 960);

    glutMainLoop();

    return NULL;
}

int main(int argc, char **argv) {
    device = &freenect.createDevice<MyFreenectDevice>(0);
    device->startVideo();
    device->startDepth();
    gl_threadfunc(NULL);
    device->stopVideo();
    device->stopDepth();
    return 0;
}
