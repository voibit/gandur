/**
 *  @file gandur.hpp
 *  Gandur, https://github.com/voibit/gandur
 *  Forked from arapaho, https://github.com/prabindh/darknet
 *  @brief C++ API for Darknet and Yolov2 Deep neural network
 *  @author Jan-Kristian Mathisen
 *  @author Joachim Lowzow
 */

#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <clocale>

#include "network.h"
#include "detection_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "region_layer.h"
#include "option_list.h"

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;
using std::cout;
using std::endl;
using std::string;
using std::vector;


/**
 * Configuration class
 * Makes stuff easier in Gandur.
 */
class Cfg {
public:
    Cfg();              ///< Constructor
    path names;         ///< Path to names file
    path weights;       ///< Path to weights file
    path netCfg;        ///< Path to network config file .cfg
    path tree;          ///< Path to tree file
    float thresh;       ///< Detection threshold
    float treeThresh;   ///< Detection tree threshold
    bool check();       ///< Check if files exists
};

/**
 * Store info about detection
 */
class Detection {
public:
    string label;   ///< class name
    int labelId;    ///< class id
    float prob;     ///< class probability
    cv::Rect box;   ///< Box coordinates of detection

    /**
     * Some constructors to easy up stuff.
     */
    Detection() : label(""), labelId(0), prob(0) {}
    Detection(const int &id, const string &l, const float &p, const cv::Rect &b)
            : label(l), labelId(id), prob(p), box(b) {}
};


/**
 * Wrapper class for the Darknet framework
 */
class Gandur
{
public:
    Gandur();
    ~Gandur();

    /**
     * The detction function
     * @param inputMat Image to detectio objects in
     * @param thresh  Threshold for detection override
     * @param tree_thresh Tree threshold for detection override
     * @return true if detections found.
     */
    bool Detect(cv::Mat inputMat, float thresh = 0, float tree_thresh = 0);

    /**
    * Resize the image to fit the given network. Keep aspect ratio
    * and fill image emerged borders with gray as darknet expects.
    * @param input image to resize
    * @return resized image
    */
    cv::Mat resizeLetterbox(const cv::Mat &input);

    /**
    * Converts opencv image to darknet friendly float array
    * @param inputMat opencv image
    * @return darknet compatible image
    */
    float *bgrToFloat(const cv::Mat &inputMat);

    /**
    * Gives label string from label id
    * @param id label id
    * @return label string
    */
    string getLabel(const unsigned int &id);

    vector<string> getClasses(); ///< Gives vector with all clasess @return class names

    /**
    * Converts darknet percent boxes to opencv pixel boxes
    * @param width image width
    * @param height image height
    * @param b Darknet box to convert
    * @return cvbox aka Rect
    */
    cv::Rect ptoi(const int &width, const int &height, const box &b);

    void setThresh(const float &f);     ///< Sets new threshold @param f threshold
    void setTreeThresh(const float &f); ///< Sets new tree threshold @param f threshold
    void loadWeights(path p); ///< Load new weights to network @param p path to weights file

    vector<Detection> detections;       ///< Vector filled after detect()
    
    bool loadVars();      ///< Initilizes darknet detection variables,

protected:
    Cfg cfg;              ///< Config file
    cv::Mat img;          ///< Image that is processed.

    //variables needed by darknet
    box *boxes;           ///<[nboxes] Array of all boxes in Darknet.
    char **classNames;    ///<[nboxes] List of class names.
    float **probs;        ///<[nboxes][classes] Array of all propabilities.
    network *net;         ///< The whole network, as specified in cfg file.
    layer l;              ///< The last layer of the network.
    float nms;            ///< ratio for supressing multiple boxes in same area
    float **masks;        ///<[nboxes][classes?] ?
    list *options;        ///<[] Options read from config file.


    //Some load functions for extra flexibility
    bool loadCfg(path p);           ///< Load config @param p path to config file.
    bool loadCfg() { loadCfg(""); } ///< Load default config


    unsigned int nboxes;  ///< Total number of detection boxes in last layer.
    int ngpus;            ///< Number of gpus.

private:
    /**
    * Lower level detection. Uses Darknet functions.
    * @param inData image to do detections in
    * @param thresh detection threshold
    * @param treeThresh detection tree threshold
    */
    void __Detect(float *inData, float thresh, float treeThresh);

};