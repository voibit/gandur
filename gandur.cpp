/*************************************************************************
 * Gandur                                                                *
 * C++ API for Yolo v2 (Detection)                                       *                                                                   *
 * Forked from arapaho, https://github.com/prabindh/darknet              *
 *************************************************************************/
#include "gandur.hpp"

Gandur::Gandur() {
    boxes = 0;
    probs = 0;
    classNames = 0;
    l = {};
    net = {};
    nms = 0.4;  //TODO: figure out what this is.... 
    threshold = 0;
    masks = 0;
    setlocale(LC_NUMERIC,"C");
    Setup();
}
    
Gandur::~Gandur() {
    // TODO - Massive cleanup here
    if(boxes) free(boxes);
    if(probs) free_ptrs((void **)probs, l.w*l.h*l.n);
    if(classNames) {
        for (size_t i = 0; i < l.classes; i++) {         
            free(classNames[i]);
        }
        free(classNames);
    }
    if (l.coords > 4){
        masks = (float**)calloc(l.w*l.h*l.n, sizeof(float*));
        for(int j = 0; j < l.w*l.h*l.n; ++j) free(masks[j]);
        free(masks);
    }
    masks=0;
    boxes = 0;
    probs = 0;
    classNames = 0;


}
    
bool Gandur::Setup() {
    list *options = read_data_cfg(CONFIG);
    char *nameListFile = option_find_str(options, (char*)"names", (char*)"data/names.list");
    char *networkFile = option_find_str(options, (char*)"networkcfg", (char*)"data/sea.cfg");
    char *weightsFile = option_find_str(options, (char*)"weights", (char*)"data/sea.weights");

    threshold = option_find_float(options, (char*)"thresh", 0.5);

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

    DPRINTF("Setup: net.n = %d\n", net->n);   
    DPRINTF("net.layers[0].batch = %d\n", net->layers[0].batch);
    
    load_weights(net, weightsFile);
    set_batch_network(net, 1);     
    l = net->layers[net->n-1];
    DPRINTF("Setup: layers = %d, %d, %d\n", l.w, l.h, l.n); 
    DPRINTF("Image expected w,h = [%d][%d]!\n", net->w, net->h);            
    
    boxes = (box*)calloc(l.w*l.h*l.n, sizeof(box));
    probs = (float**)calloc(l.w*l.h*l.n, sizeof(float *));

    if (l.coords > 4){
        masks = (float**)calloc(l.w*l.h*l.n, sizeof(float*));
        for(int j = 0; j < l.w*l.h*l.n; ++j) masks[j] = (float*)calloc(l.coords-4, sizeof(float));
    }
    
    // Error exits
    if(!boxes || !probs) {
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

bool Gandur::Detect(const cv::Mat &inputMat,float thresh, float tree_thresh){
    image = inputMat;
    int i, count=0;
    xScale=1;
    yScale=1;    
    detections.clear();

    if(inputMat.empty()) {
        EPRINTF("Error in inputImage! [bgr = %d, w = %d, h = %d]\n",
            !inputMat.data, inputMat.cols != net->w,
            inputMat.rows != net->h);
        return false;
    }

    //Convert to rgb
    cv::Mat inputRgb;
    cvtColor(inputMat, inputRgb, CV_BGR2RGB);
    // Convert the bytes to float
    cv::Mat floatMat;

    if (inputRgb.rows != net->h || inputRgb.cols != net->w) { 
        inputRgb=resizeKeepAspectRatio(inputRgb, cv::Size(net->w, net->h), cv::Scalar(128, 128,128));
    }

    inputRgb.convertTo(floatMat, CV_32FC3, 1/255.0);

    // Get the image to suit darknet
    std::vector<cv::Mat> floatMatChannels(3);
    split(floatMat, floatMatChannels);
    vconcat(floatMatChannels, floatMat);

    __Detect((float*)floatMat.data, thresh > 0 ? thresh:threshold, tree_thresh);

    return true;
}

int Gandur::getLabelId(const std::string &name) {

    for (size_t i = 0; i < l.classes; i++) {
        if (std::string(classNames[i]) == name) return i;         
    }
    return -1; 
}
std::string Gandur::getLabel(const unsigned int id) {
    if (id < l.classes) {
        return std::string(classNames[id]);
    }
    else return "error";
}
    
cv::Mat Gandur::resizeKeepAspectRatio(
    const cv::Mat &input,
    const cv::Size &dstSize,
    const cv::Scalar &bgcolor){

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

std::vector<std::string> Gandur::getClasses() {
    std::vector<std::string> v(classNames, classNames+l.classes);
    return v;
}

//////////////////////////////////////////////////////////////////
/// Private APIs
//////////////////////////////////////////////////////////////////
void Gandur::__Detect(float* inData, float thresh, float tree_thresh) {
    int i;
    // Predict
    network_predict(net, inData);

    // for latest commit
    get_region_boxes(l, 1, 1,net->w, net->h, thresh, probs, boxes, masks, 0, 0, tree_thresh,1);

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

        //print all propabilities
        for(size_t j = 0; j < l.classes; j++) {
            if (probs[i][j] > 0.2) {
                printf("%s: %.0f%%\n", classNames[j], probs[i][j]*100);

            }
        }

    }
}