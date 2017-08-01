/*************************************************************************
 * arapaho                                                               *
 *                                                                       *
 * C++ API for Yolo v2 (Detection)                                       *
 *                                                                       *
 * https://github.com/prabindh/darknet                                   *
 *                                                                       *
 * Forked from, https://github.com/pjreddie/darknet                      *
 *                                                                       *
 *************************************************************************/

#ifndef _ENABLE_ARAPAHO
#define _ENABLE_ARAPAHO

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <string>
#include <opencv2/opencv.hpp>
#include "network.h"
#include "detection_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "region_layer.h"
#include "option_list.h"

#include <opencv2/tracking.hpp>

//////////////////////////////////////////////////////////////////////////

#define ARAPAHO_MAX_CLASSES (200)

#ifdef _DEBUG
#define DPRINTF printf
#define EPRINTF printf
#else
#define DPRINTF
#define EPRINTF printf
#endif

    //////////////////////////////////////////////////////////////////////////

    struct ArapahoV2Params
    {
        char* datacfg;
        char* cfgfile;
        char* weightfile;
        float nms;
        int maxClasses;
    };

    struct ArapahoV2ImageBuff
    {
        unsigned char* bgr;
        int w;
        int h;
        int channels;
    };
    //////////////////////////////////////////////////////////////////////////

    class ArapahoV2
    {
    public:
        ArapahoV2();
        ~ArapahoV2();


        bool Setup(ArapahoV2Params & p,
            int & expectedWidth,
            int & expectedHeight);

        bool Detect(
            ArapahoV2ImageBuff & imageBuff,
            float thresh,
            float hier_thresh,
            int & objectCount);

        bool Detect(
            const cv::Mat & inputMat,
            float thresh,
            float hier_thresh,
            int & objectCount);

        bool GetBoxes(box* outBoxes, std::string* outLabels, float* prob, int boxCount);
        cv::Mat resizeKeepAspectRatio(const cv::Mat &input, const cv::Size &dstSize, const cv::Scalar &bgcolor);


    private:
        box     *boxes;
        char    **classNames;
        float   **probs;
        bool    bSetup;
        network net;
        layer   l;
        float   nms;
        int     maxClasses;
        int     threshold;

        float   xScale;
        float   yScale;

        void __Detect(float* inData, float thresh, float hier_thresh, int & objectCount);
    };

    //////////////////////////////////////////////////////////////////////////



inline cv::Ptr<cv::Tracker> createTrackerByName(cv::String name)
{
    cv::Ptr<cv::Tracker> tracker;

    if (name == "KCF")
        tracker = cv::TrackerKCF::create();
    else if (name == "TLD")
        tracker = cv::TrackerTLD::create();
    else if (name == "BOOSTING")
        tracker = cv::TrackerBoosting::create();
    else if (name == "MEDIAN_FLOW")
        tracker = cv::TrackerMedianFlow::create();
    else if (name == "MIL")
        tracker = cv::TrackerMIL::create();
    else if (name == "GOTURN")
        tracker = cv::TrackerGOTURN::create();
    else
        CV_Error(cv::Error::StsBadArg, "Invalid tracking algorithm name\n");

    return tracker;
}


#endif // _ENABLE_ARAPAHO
