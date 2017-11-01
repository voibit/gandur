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

#include <boost/filesystem.hpp>

#ifdef _DEBUG
#define DPRINTF printf
#define EPRINTF printf
#else
#define DPRINTF
#define EPRINTF printf
#endif


using namespace boost::filesystem;
using std::cout;
using std::endl;
using std::string;
using std::vector; 

struct Detection {
    string label;
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
    bool Detect(cv::Mat inputMat,float thresh=0, float tree_thresh=0.5);

    cv::Mat resizeLetterbox(const cv::Mat &input);

    cv::Mat bgrToFloat(const cv::Mat &inputMat);

    vector<Detection> detections;

    int getLabelId(const string &name);
    string getLabel(const unsigned int id);
    vector<string> getClasses();
    cv::Rect ptoi(const int &width, const int &height, const box &b);
    bool validate();

private:
    cv::Mat img; 
    box     *boxes;
    char    **classNames;
    float   **probs;
    network *net;
    layer   l;
    float   thresh;
    float   nms;
    float **masks;
    boost::filesystem::path configFile;
    void __Detect(float* inData, float thresh, float tree_thresh);
    list *options;
    char *networkFile;
};

#endif // _ENABLE_GANDUR