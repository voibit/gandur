/*************************************************************************
 * Gandur, https://github.com/voibit/gandu                               *
 * C++ API for Yolo v2                                                   *                                                                   *
 * Forked from arapaho, https://github.com/prabindh/darknet              *
 *************************************************************************/
#ifndef _ENABLE_GANDUR
#define _ENABLE_GANDUR

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <clocale>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
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

using namespace boost::filesystem;
using std::cout;
using std::endl;
using std::string;
using std::vector;

class Cfg {
public:
    Cfg();

    path names;
    path weights;
    path netCfg;
    path tree;
    float thresh;
    float treeThresh;

    bool check();
};

class Detection {
public:
    string label;
    int labelId;
    float prob;
    cv::Rect box;

    Detection() : label(""), labelId(0), prob(0) {}
    Detection(const int &id, const string &l, const float &p, const cv::Rect &b)
            : label(l), labelId(id), prob(p), box(b) {}
};

class Gandur
{
public:
    Gandur();
    ~Gandur();

    bool Detect(cv::Mat inputMat, float thresh = 0, float tree_thresh = 0);
    cv::Mat resizeLetterbox(const cv::Mat &input);
    cv::Mat bgrToFloat(const cv::Mat &inputMat);
    vector<Detection> detections;
    string getLabel(const unsigned int &id);
    vector<string> getClasses();
    cv::Rect ptoi(const int &width, const int &height, const box &b);

    void setThresh(const float &f);

    void setTreeThresh(const float &f);

    void loadWeights(path p);

protected:
    Cfg cfg;
    cv::Mat img; 
    box     *boxes;
    char    **classNames;
    float   **probs;
    network *net;
    layer   l;
    float   nms;
    float **masks;
    list *options;

    bool loadCfg(path p);

    bool loadCfg() { loadCfg(""); }

    bool loadVars();

    unsigned int nboxes;

private:
    void __Detect(float *inData, float thresh, float tree_thresh);


};

#endif // _ENABLE_GANDUR