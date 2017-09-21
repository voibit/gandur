/*************************************************************************
 * Gandur                                                                *
 * C++ API for Yolo v2 (Detection)                                       *                                                                   *
 * Forked from, https://github.com/prabindh/darknet                      *
 *************************************************************************/
#include "gandur.hpp"

Gandur::Gandur()
{
    boxes = 0;
    probs = 0;
    classNames = 0;
    l = {};
    net = {};
    nms = 0.4;  //TODO: figure out what this is.... 
    threshold = 0;
    setlocale(LC_NUMERIC,"C");
    Setup();
}
    
Gandur::~Gandur()
{
    // TODO - Massive cleanup here
    
    if(boxes) 
        free(boxes);
    if(probs)
        free_ptrs((void **)probs, l.w*l.h*l.n);
    if(classNames)
    {
    //todo
    }
    boxes = 0;
    probs = 0;
    classNames = 0;
}
    
bool Gandur::Setup()
{

    list *options = read_data_cfg(CONFIG);
    char *nameListFile = option_find_str(options, (char*)"names", (char*)"data/names.list");
    char *networkFile = option_find_str(options, (char*)"networkcfg", (char*)"data/sea.cfg");
    char *weightsFile = option_find_str(options, (char*)"weights", (char*)"data/sea.weights");

    if(!nameListFile){
        DPRINTF("No valid nameList file specified in options file [%s]!\n", CONFIG);
        return false;
    }

    classNames = get_labels(nameListFile);

    if(!classNames){
        DPRINTF("No valid class names specified in nameList file [%s]!\n", nameListFile);
        return false;
    }

    int j;
    // Early exits
    if(!networkFile) {
        EPRINTF("No cfg file specified!\n");
        return false;
    }
    if(!weightsFile) {
        EPRINTF("No weights file specified!\n");
        return false;
    }    
    // Print some debug info
    net = parse_network_cfg(networkFile);

    DPRINTF("Setup: net.n = %d\n", net.n);   
    DPRINTF("net.layers[0].batch = %d\n", net.layers[0].batch);
    
    load_weights(&net, weightsFile);
    set_batch_network(&net, 1);     
    l = net.layers[net.n-1];
    DPRINTF("Setup: layers = %d, %d, %d\n", l.w, l.h, l.n); 
    DPRINTF("Image expected w,h = [%d][%d]!\n", net.w, net.h);            
    
    boxes = (box*)calloc(l.w*l.h*l.n, sizeof(box));
    probs = (float**)calloc(l.w*l.h*l.n, sizeof(float *));
    
    // Error exits
    if(!boxes || !probs)
    {
        EPRINTF("Error allocating boxes/probs, %p/%p !\n", boxes, probs);
        goto clean_exit;
    }

    for(j = 0; j < l.w*l.h*l.n; ++j) 
    {
        probs[j] = (float*)calloc(l.classes + 1, sizeof(float));
        if(!probs[j])
        {
            EPRINTF("Error allocating probs[%d]!\n", j);            
            goto clean_exit;
        }
    }
    DPRINTF("Setup: Done\n");
    return true;
    
clean_exit:        
    if(boxes) 
        free(boxes);
    if(probs)
        free_ptrs((void **)probs, l.w*l.h*l.n);
    boxes = NULL;
    probs = NULL;
    return false;
}


bool Gandur::Detect(
            const cv::Mat &inputMat,
            float thresh, 
            float hier_thresh=0.5)
{

    image = inputMat;
    int i, count=0;
    xScale=1;
    yScale=1;    
    threshold = thresh;

    detections.clear();

    if(inputMat.empty())
    {
        EPRINTF("Error in inputImage! [bgr = %d, w = %d, h = %d]\n",
            !inputMat.data, inputMat.cols != net.w,
            inputMat.rows != net.h);
        return false;
    }

    //Convert to rgb
    cv::Mat inputRgb;
    cvtColor(inputMat, inputRgb, CV_BGR2RGB);
    // Convert the bytes to float
    cv::Mat floatMat;

    if (inputRgb.rows != net.h || inputRgb.cols != net.w) { 
        inputRgb=resizeKeepAspectRatio(inputRgb, cv::Size(net.w, net.h), cv::Scalar(128, 128,128));
    }

    inputRgb.convertTo(floatMat, CV_32FC3, 1/255.0);

    // Get the image to suit darknet
    std::vector<cv::Mat> floatMatChannels(3);
    split(floatMat, floatMatChannels);
    vconcat(floatMatChannels, floatMat);

    __Detect((float*)floatMat.data, thresh, hier_thresh);

    return true;
}

int Gandur::getLabelId(const std::string &name) {

    for (size_t i = 0; i < l.classes; i++) {
        if (std::string(classNames[i]) == name) return i;         
    }
    return -1; 
}
    
cv::Mat Gandur::resizeKeepAspectRatio(const cv::Mat &input, const cv::Size &dstSize, const cv::Scalar &bgcolor)
{
    cv::Mat output;

    double h1 = dstSize.width * (input.rows/(double)input.cols);
    double w2 = dstSize.height * (input.cols/(double)input.rows);
    if( h1 <= dstSize.height) {
        cv::resize( input, output, cv::Size(dstSize.width, h1));
        yScale=dstSize.height / h1;
    } else {
        cv::resize( input, output, cv::Size(w2, dstSize.height));
        xScale=dstSize.width / w2;
    }

    int top = (dstSize.height-output.rows) / 2;
    int down = (dstSize.height-output.rows+1) / 2;
    int left = (dstSize.width - output.cols) / 2;
    int right = (dstSize.width - output.cols+1) / 2;

    cv::copyMakeBorder(output, output, top, down, left, right, cv::BORDER_CONSTANT, bgcolor );

    return output;
}

cv::Mat Gandur::drawDetections() {
    using namespace cv; 

    cv::Mat img = image.clone(); 

    for(Detection det : detections) {
        /*
        //Calculate degrees.
        float dpp = aov / sqrt(image.cols*image.cols + image.rows* image.rows) ;
        float degrees = o.rects[i].x+o.rects[i].width/2-image.cols/2;
        degrees *= dpp;
        */

        //DPRINTF("Label:%s prob: %00f \n\n", labels[objId].c_str(),probs[objId]);
        
        //Draw label
        char prob[5];
        sprintf(prob,"%1.2f",det.prob);

        Point posLabel=det.box.tl();  //top left

        putText(img, det.label, posLabel, FONT_HERSHEY_DUPLEX, 1, CV_RGB(0, 0, 0), 1, CV_AA);

        posLabel.x-=2;
        posLabel.y-=1;

        putText(img, det.label, posLabel, FONT_HERSHEY_DUPLEX, 1, CV_RGB(70, 250, 20), 1, CV_AA);   

        posLabel.x-=55;

        putText(img, prob, posLabel,FONT_HERSHEY_DUPLEX, 0.7,CV_RGB(0, 0, 0),1, CV_AA);
        
        posLabel.x-=1;
        posLabel.y-=1;

        putText(img, prob, posLabel, FONT_HERSHEY_DUPLEX, 0.7,CV_RGB(70, 200, 0),1, CV_AA);
        
        //Draw line from bottom 
        Point posCircle = Point(det.box.x+det.box.width/2, det.box.y+det.box.height);
        Point posDegree = Point(det.box.x, det.box.y+det.box.height+14);
        
        // Draw sircle and line
        circle(img, posCircle, 4, cvScalar(50, 255, 200), 2);
        line(img, cvPoint(img.cols/2-1, img.rows), posCircle,cvScalar(0, 0, 0), 2);
        line(img, cvPoint(img.cols/2+1, img.rows),posCircle, CV_RGB(100, 200, 255), 1);
        // Show image and overlay using OpenCV
        
        rectangle(img, det.box,CV_RGB(100, 200, 255), 1, 8, 0);

        /*
        //Draw bearing from center
        char str[9]; 
        sprintf(str,"%3.2f", degrees);

        posDegree.x+=50;
        putText(img, str, posDegree,
                FONT_HERSHEY_DUPLEX, 1.3, CV_RGB(0, 0, 0), 2, CV_AA);
        posDegree.x-=2;
        posDegree.y-=1;         
        putText(img, str, posDegree,
                FONT_HERSHEY_DUPLEX, 1.3, CV_RGB(70, 250, 20), 1, CV_AA);
        */
    }
    return img;
}

std::vector<std::string> Gandur::getClasses() {
    std::vector<std::string> v(classNames, classNames+l.classes);
    return v;
}

//////////////////////////////////////////////////////////////////
/// Private APIs
//////////////////////////////////////////////////////////////////
void Gandur::__Detect(float* inData, float thresh, float hier_thresh)
{
    int i;
    // Predict
    network_predict(net, inData);
    get_region_boxes(l, 1, 1,net.w, net.h, thresh, probs, boxes, 0, 0, hier_thresh,1);

    DPRINTF("l.softmax_tree = %p, nms = %f\n", l.softmax_tree, nms);
    if (l.softmax_tree && nms)
    {
        do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
    }
    else if (nms)
        do_nms_sort(boxes, probs, l.w*l.h*l.n, l.classes, nms);

    // Update object counts
    for (i = 0; i < (l.w*l.h*l.n); ++i){

        int class1 = max_index(probs[i], l.classes);
        float prob = probs[i][class1];

        if (prob > thresh){

            DPRINTF("\nX:%f, Y:%f xscale:%f, yscale%f\n",boxes[i].x, boxes[i].y,xScale, yScale);
            Detection tmp;

            tmp.label= std::string(classNames[class1]);
            tmp.prob=prob;
            tmp.labelId=class1;
            tmp.box.width=(double)image.cols* boxes[i].w*xScale;
            tmp.box.height=(double)image.rows* boxes[i].h*yScale;

            //convert x and y from letterbox to actual coordinates
            tmp.box.x =image.cols* (boxes[i].x * xScale - ((xScale-1)/2.)) -tmp.box.width/2;
            tmp.box.y =image.rows* (boxes[i].y * yScale - ((yScale-1)/2.)) - tmp.box.height/2;

            DPRINTF("Object:%s w:%ipx h%ipx \n w:%f h:%f x:%f y:%f", tmp.label.c_str(), image.cols, image.rows, boxes[i].w, boxes[i].h,boxes[i].x, boxes[i].y);
            DPRINTF("\n w:%i h:%i x:%i y:%i\n", tmp.box.width, tmp.box.height, tmp.box.x, tmp.box.y);
            detections.push_back(tmp);
        }
    }
}