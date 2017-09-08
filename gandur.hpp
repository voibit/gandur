/*************************************************************************
 * Gandur                                                                *
 *                                                                       *
 * C++ API for Yolo v2 (Detection)                                       *
 *                                                                       *                                                                      *
 * Forked from, https://github.com/prabindh/darknet                      *
 *                                                                       *
 *************************************************************************/

#ifndef _ENABLE_GANDUR
#define _ENABLE_GANDUR

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "network.h"
#include "detection_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "region_layer.h"
#include "option_list.h"

#ifdef _DEBUG
#define DPRINTF printf
#define EPRINTF printf
#else
#define DPRINTF
#define EPRINTF printf
#endif

static char CONFIG[]    = "gandur.conf"; 

struct Detection {
    std::string label;
    float prob;
    cv::Rect box;
};

class Gandur
{
public:
    Gandur();
    ~Gandur();
    bool Setup();
    bool Detect(
        const cv::Mat & inputMat,
        float thresh,
        float hier_thresh);

    cv::Mat drawDetections();

    cv::Mat resizeKeepAspectRatio(
        const cv::Mat &input,
        const cv::Size &dstSize,
        const cv::Scalar &bgcolor);

    std::vector<Detection> detections;

private:
    cv::Mat image; 
    box     *boxes;
    char    **classNames;
    float   **probs;
    network net;
    layer   l;
    float   nms;
    int     maxClasses;
    int     threshold;

    float   xScale;
    float   yScale;

    void __Detect(float* inData, float thresh, float hier_thresh);
};

#endif // _ENABLE_GANDUR