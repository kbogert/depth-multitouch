/*
 * library for detecting touches on a flat surface using depth maps.
 *
 *
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <opencv/cv.h>

#define D_MULTITOUCH_OK 0
#define D_MULTITOUCH_ERROR 1

//#define D_MULTITOUCH_DEBUG_TOUCH 1



struct d_multitouch_touch {
    int x;
    int y;
    double r;
    struct d_multitouch_touch * next;
} d_multitouch_touch;


typedef struct _d_multitouch_data {

    int height_of_touch;
    int img_width;
    int img_height;
    uint16_t * base_depthmap;
    uint16_t * cur_delta_map;
    uint16_t * delta_map;
    IplImage * touch_map;
    IplImage * connected_regions;
    IplImage * connected_regions_filter;

    double x;
    double y;
    double r;

    struct d_multitouch_touch * cur_touches;

} _d_multitouch_data;



void * d_multitouch_init();
void d_multitouch_reset(void * data);
void d_multitouch_free_basemap(void * data);
void d_multitouch_set_base(uint16_t * depthmap, int width, int height, void * data);
void d_multitouch_set_height_of_touch(int units, void * data);
void d_multitouch_update(uint16_t * depthmap, void * data);
uint8_t * d_multitouch_get_touch_map(void * data);


/* Should be called once at the start of the program */
void * d_multitouch_init() {
    _d_multitouch_data * returnval = (_d_multitouch_data *) malloc(sizeof(_d_multitouch_data));
    returnval->img_width = 0;
    returnval->img_height = 0;
    returnval->height_of_touch = 0;
    returnval->delta_map = 0;
    returnval->cur_delta_map = 0;
    returnval->cur_touches = 0;
    returnval->touch_map = 0;
    returnval->connected_regions = 0;
    returnval->connected_regions_filter = 0;
    d_multitouch_reset(returnval);
    return returnval;
}

/* Called to reset the delta and base maps */
void d_multitouch_reset(void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    multitouch_data->base_depthmap = 0;
    if (multitouch_data->delta_map != 0)
        free(multitouch_data->delta_map);
    if (multitouch_data->touch_map != 0)
        cvReleaseImage(&multitouch_data->touch_map);

    if (multitouch_data->connected_regions != 0)
        cvReleaseImage(&multitouch_data->connected_regions);
    if (multitouch_data->connected_regions_filter != 0)
        cvReleaseImage(&multitouch_data->connected_regions_filter);
    multitouch_data->connected_regions = cvCreateImage(cvSize(multitouch_data->img_width, multitouch_data->img_height), IPL_DEPTH_8U, 3);
    multitouch_data->connected_regions_filter = cvCreateImage(cvSize(multitouch_data->img_width, multitouch_data->img_height), IPL_DEPTH_8U, 3);
    multitouch_data->delta_map = (uint16_t *) malloc(sizeof(uint16_t) * multitouch_data->img_width * multitouch_data->img_height);
}

/* Called to free the given base map */
void d_multitouch_free_basemap(void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    if (multitouch_data->base_depthmap != 0) {
        free(multitouch_data->base_depthmap);
    }
    d_multitouch_reset(data);
}

void d_multitouch_set_base(uint16_t * depthmap, int width, int height, void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    multitouch_data->img_width = width;
    multitouch_data->img_height = height;
    d_multitouch_free_basemap(data);
    multitouch_data->base_depthmap = (uint16_t*)malloc(sizeof(uint16_t) * width*height);
    memcpy(multitouch_data->base_depthmap, depthmap, sizeof(uint16_t) * width * height);
}

void d_multitouch_set_height_of_touch(int units, void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    multitouch_data->height_of_touch = units;
}

// the fit circle algorithm by Dr. Kyle Johnsen
void fitCircle(CvSeq * contour, double * xp, double * yp, double * r){
    int i;
	CvMat* A  = cvCreateMat(contour->total,3,CV_32FC1);
	CvMat* x  = cvCreateMat(3,1,CV_32FC1);
	CvMat* b  = cvCreateMat(contour->total,1,CV_32FC1);
	for(i =0;i<contour->total;i++){
		CvPoint *p = (CvPoint *)cvGetSeqElem(contour,i);
		cvmSet(A,i,0,p->x); // Set M(i,j)
		cvmSet(A,i,1,p->y);
		cvmSet(A,i,2,1);
		cvmSet(b,i,0,-(p->x*p->x)-(p->y*p->y));
	}
	CvMat* At = cvCreateMat(3,contour->total,CV_32FC1);
	CvMat* Z = cvCreateMat(3,3,CV_32FC1);
	CvMat* q = cvCreateMat(3,1,CV_32FC1);
	cvmTranspose(A,At);
	cvmMul(At,A,Z);
	cvmMul(At,b,q);
	cvSolve(Z, q, x, CV_LU);    // solve (Ax=b) for x
	double a,bb,c;

	a = cvmGet(x,0,0);
	bb = cvmGet(x,1,0);
	c = cvmGet(x,2,0);
	(*xp) = -a/2;
	(*yp) = -bb/2;
	(*r) = sqrt((*xp)*(*xp)+(*yp)*(*yp) - c);
	cvReleaseMat(&A);
	cvReleaseMat(&x);
	cvReleaseMat(&b);
	cvReleaseMat(&At);
	cvReleaseMat(&Z);
	cvReleaseMat(&q);
}

int _d_multitouch_valid_touch_test (uint16_t * depthmap, int x, int y, void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;

    int maxCutoff = 25;
    int maxJump = 10;

    int prevX, prevY;
    int nextX, nextY;
    int i, j, posToCheck, diff, maxdiff;

    nextX = x;
    nextY = y;
    while (depthmap[nextY * multitouch_data->img_width + nextX] < maxCutoff) {

        prevX = nextX;
        prevY = nextY;
        maxdiff = 0;

        for (i = -10; i < 10; i ++) {
            for (j = -10; j < 10; j ++) {
                if (prevX + i < 0 ||
                    prevX + i >= multitouch_data->img_width ||
                    prevY + j < 0 ||
                    prevY + j >= multitouch_data->img_height)

                    continue; // off the edge of the image

                posToCheck = (prevY + j) * multitouch_data->img_width + (prevX + i);

                diff = depthmap[posToCheck] - depthmap[nextY * multitouch_data->img_width + nextX];
                if (diff > maxdiff && diff < maxJump) {
                    maxdiff = diff;
                    nextX = prevX + i;
                    nextY = prevY + j;
                }

            }
        }
        ((uchar*)(multitouch_data->connected_regions_filter->imageData + multitouch_data->connected_regions_filter->widthStep*nextY))[nextX*3+1] = 0;
        ((uchar*)(multitouch_data->connected_regions_filter->imageData + multitouch_data->connected_regions_filter->widthStep*nextY))[nextX*3+2] = 0;

        if (prevX == nextX && prevY == nextY) {
            // failed to find a higher pixel
            return 0;
        }

    }

    if (depthmap[nextY * multitouch_data->img_width + nextX] > 100)
        return 0;

//    printf(" %d, %d: %d\n", x, y, depthmap[nextY * multitouch_data->img_width + nextX] );

    return 1;
}

void _d_multitouch_clear_touches(struct d_multitouch_touch * head) {
    if (head != 0) {
        _d_multitouch_clear_touches(head->next);
    }

    free(head);
}

void d_multitouch_update(uint16_t * depthmap, void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    if (multitouch_data->base_depthmap == 0) {
        multitouch_data->cur_delta_map = depthmap;
        return;
    }

    multitouch_data->cur_delta_map = multitouch_data->delta_map;
    CvMat * temp = cvCreateMat(multitouch_data->img_height, multitouch_data->img_width, CV_32FC1);

    CvMat * thresholdedHeight = cvCreateMat(multitouch_data->img_height, multitouch_data->img_width, CV_8UC1);

    int i;
    for (i = 0; i < multitouch_data->img_width * multitouch_data->img_height; i ++) {

        multitouch_data->delta_map[i] = multitouch_data->base_depthmap[i] - depthmap[i];
        //printf("%d - %d -> %d ", multitouch_data->base_depthmap[i] - depthmap[i]multitouch_data->delta_map[i]);
        if (multitouch_data->delta_map[i] <= 3 || multitouch_data->delta_map[i] > multitouch_data->height_of_touch) {
//            multitouch_data->delta_map[i] = 0;
            cvmSet(temp, i / multitouch_data->img_width, i % multitouch_data->img_width, 0);

        } else {
//            multitouch_data->delta_map[i] = (1 - (multitouch_data->delta_map[i] / (double)multitouch_data->height_of_touch)) * 2000;
            cvmSet(temp, i / multitouch_data->img_width, i % multitouch_data->img_width, 2);

        }
    }


    cvInRangeS( temp, cvRealScalar(1), cvRealScalar(3), thresholdedHeight );

    //find image contours
    CvMemStorage* storage = cvCreateMemStorage(0);
    CvSeq *contours = NULL;
    int numFound = cvFindContours(thresholdedHeight,storage,&contours,sizeof(CvContour),CV_RETR_CCOMP,CV_CHAIN_APPROX_NONE,cvPoint(0,0) );

    _d_multitouch_clear_touches(multitouch_data->cur_touches);
    struct d_multitouch_touch * touch_list;
    struct d_multitouch_touch * tempTouch;
    touch_list = 0;


    //find the center and radius
    double x = 0,y=0,r = 0;
    if(numFound > 0 ){

    cvRectangle(multitouch_data->connected_regions, cvPoint(0,0), cvPoint(multitouch_data->img_width, multitouch_data->img_height), cvScalar(255,255,255,0), CV_FILLED, 8, 0);
    cvRectangle(multitouch_data->connected_regions_filter, cvPoint(0,0), cvPoint(multitouch_data->img_width, multitouch_data->img_height), cvScalar(255,255,255,0), CV_FILLED, 8, 0);

        for(i=0;i<numFound && contours!=NULL;i++){
            if (contours->total > 15) {
                CvRect boundingBox = cvBoundingRect(contours, 0);

                cvDrawContours(multitouch_data->connected_regions, contours, cvScalar(0,0,255,0), cvScalar(255,255,255,0), -1, CV_FILLED, 8, cvPoint(0,0));
                if (_d_multitouch_valid_touch_test (multitouch_data->delta_map, boundingBox.x + (boundingBox.width / 2), boundingBox.y + (boundingBox.height / 2), data)) {

                    cvDrawContours(multitouch_data->connected_regions_filter, contours, cvScalar(0,0,255,0), cvScalar(255,255,255,0), -1, CV_FILLED, 8, cvPoint(0,0));

                    fitCircle(contours,&x,&y,&r);
                    if ( (int)r > 1 && (int)r < 25) {
                        tempTouch = (struct d_multitouch_touch *) malloc(sizeof(struct d_multitouch_touch));
                        tempTouch->next = touch_list;
                        tempTouch->r = r;
                        tempTouch->x = x;
                        tempTouch->y = y;
                        touch_list = tempTouch;
                    }
                }
            }
            contours = contours->h_next;
        }
    }

    multitouch_data->cur_touches = touch_list;


    cvReleaseMemStorage(&storage);

    cvReleaseMat(&thresholdedHeight);
    cvReleaseMat(&temp);

}

uint8_t * d_multitouch_get_touch_map(void * data) {
    // create a 3 channel blank image
    // draw circles for each of the current touches
    // return the data
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    if (multitouch_data->touch_map == 0)
         multitouch_data->touch_map = cvCreateImage(cvSize(multitouch_data->img_width, multitouch_data->img_height), IPL_DEPTH_8U, 3);
#ifndef D_MULTITOUCH_DEBUG_TOUCH
    cvRectangle(multitouch_data->touch_map, cvPoint(0,0), cvPoint(multitouch_data->img_width, multitouch_data->img_height), cvScalar(255,255,255,0), CV_FILLED, 8, 0);

    struct d_multitouch_touch * cur = multitouch_data->cur_touches;
    while (cur != 0) {
        cvCircle(multitouch_data->touch_map, cvPoint(cur->x, cur->y), cur->r, cvScalar(0,0,255,0), CV_FILLED, 8, 0);
        cur = cur->next;
    }
#endif
    return (uint8_t *)multitouch_data->touch_map->imageData;
}

uint16_t * d_multitouch_get_depth_difference(void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    return multitouch_data->cur_delta_map;
}

uint8_t * d_multitouch_get_connected_regions(void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    return (uint8_t *)multitouch_data->connected_regions->imageData;
}

uint8_t * d_multitouch_get_depth_filter_debug(void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    return (uint8_t *)multitouch_data->connected_regions_filter->imageData;
}

struct d_multitouch_touch * d_multitouch_get_touches(void * data) {
    _d_multitouch_data * multitouch_data = (_d_multitouch_data *) data;
    return multitouch_data->cur_touches;
}
