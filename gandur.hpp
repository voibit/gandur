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

static char CONFIG[] = "/home/ed15/gandur/gandur.conf"; 

struct Detection {
    std::string label;
    unsigned int labelId; 
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
        float thresh=0,
        float tree_thresh=0.5);

    cv::Mat drawDetections();

    cv::Mat resizeKeepAspectRatio(
        const cv::Mat &input,
        const cv::Size &dstSize,
        const cv::Scalar &bgcolor);

    std::vector<Detection> detections;

    int getLabelId(const std::string &name);
    std::string getLabel(const unsigned int id);
    std::vector<std::string> getClasses();

private:
    cv::Mat image; 
    box     *boxes;
    char    **classNames;
    float   **probs;
    network net;
    layer   l;
    float   threshold;
    float   nms;
    float   xScale;
    float   yScale;

    void __Detect(float* inData, float thresh, float tree_thresh);
};

#endif // _ENABLE_GANDUR