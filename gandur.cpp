/*************************************************************************
 * Gandur, https://github.com/voibit/gandu                               *
 * C++ API for Yolo v2                                                   *                                                                   *
 * Forked from arapaho, https://github.com/prabindh/darknet              *
 *************************************************************************/
#include "gandur.hpp"


Gandur::Gandur() {
    nboxes = 0;
    boxes = nullptr;
    probs = nullptr;
    classNames = nullptr;
    l = {};
    net = {};
    nms = 0.4; // Default value used in darknet
    masks = nullptr;
    ngpus = 1;
    setlocale(LC_NUMERIC,"C");
    loadCfg("gandur.conf");
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
        for(int j = 0; j < l.w*l.h*l.n; ++j) free(masks[j]);
        free(masks);
    }
    masks = nullptr;
    boxes = nullptr;
    probs = nullptr;
    classNames = nullptr;
}

bool Gandur::loadCfg(path p) {
    if (p.empty() || !is_regular_file(p)) {
        cout << "No config" << p << "file found in this path, trying default values\n";
        options = nullptr;
    } else options = read_data_cfg((char *) p.c_str());

    cfg.names = option_find_str(options, (char *) "names", (char *) "data/names.list");
    cfg.netCfg = option_find_str(options, (char *) "networkcfg", (char *) "data/net.cfg");
    cfg.weights = option_find_str(options, (char *) "weights", (char *) "data/net.weights");
    cfg.thresh = option_find_float(options, (char *) "thresh", 0.5);
    cfg.treeThresh = option_find_float(options, (char *) "tree-thresh", 0.5);

    if (cfg.check()) {
        classNames = get_labels((char *) cfg.names.c_str());
        cfg.netCfg = canonical(cfg.netCfg);

        return true;
    } else return false;
}

void Gandur::loadWeights(path p) {
    load_weights(net, (char *) p.c_str());
}

bool Gandur::loadVars() {
    //return true if already loaded.
    if (probs != nullptr) {
        return true;
    }
    size_t j;
    net = parse_network_cfg((char *) cfg.netCfg.c_str());
    //Set layer to detection layer
    l = net->layers[net->n - 1];
    //DPRINTF("Setup: net.n = %d\n", net->n);
    //DPRINTF("net.layers[0].batch = %d\n", net->layers[0].batch);
    loadWeights(cfg.weights);

    //so you dont have to change config from trainig.
    set_batch_network(net, 1);
    net->subdivisions = 1;

    //Set boxes;
    nboxes = (unsigned int) (l.w * l.h * l.n);
    boxes = (box *) calloc(nboxes, sizeof(box));
    probs = (float **) calloc(nboxes, sizeof(float *));

    //From darknet, not shure what it does yet.
    if (l.coords > 4) {
        masks = (float **) calloc(nboxes, sizeof(float *));
        for (j = 0; j < nboxes; ++j) masks[j] = (float *) calloc(l.coords - 4, sizeof(float));
    }
    // Error exits
    if(!boxes || !probs) {
        cout << "Error allocating boxes/probs! \n";  // boxes, probs);
        goto clean_exit;
    }
    for (j = 0; j < nboxes; ++j) {
        probs[j] = (float*)calloc(l.classes + 1, sizeof(float));
        if (!probs[j]) {
            cout << "Error allocating prob in probs! \n";  // [%d]!\n", j);
            goto clean_exit;
        }
    }
    cout << "Setup: Done\n";
    return true;

    clean_exit:
    if (boxes)
        free(boxes);
    if(probs)
        free_ptrs((void **) probs, nboxes);
    boxes = nullptr;
    probs = nullptr;
    return false;
}

bool Gandur::Detect(cv::Mat inputMat, float thresh, float tree_thresh) {
    /**
     * Load vars so Darknet can store detections somewhere.
     * (if not already done).
     */
    loadVars();

    img = inputMat.clone();
    detections.clear();

    if(inputMat.empty()) {
        return false;
    }
    /**
     * Check if image is the size of the network
     * if not resize.
     */
    if (inputMat.rows != net->h || inputMat.cols != net->w) {
        inputMat=resizeLetterbox(inputMat);
    }

    /**
     * Convert input image to darknet format
     * Override thresholds if specified.
     */
    __Detect(bgrToFloat(inputMat),
             thresh == 0 ? cfg.thresh : thresh,
             tree_thresh == 0 ? cfg.treeThresh : tree_thresh);
    return !detections.empty(); ///> Return true if there are detections in image
}

float *Gandur::bgrToFloat(const cv::Mat &inputMat) {

    cv::Mat inputRgb;                           ///> Mat to store rgb image
    cvtColor(inputMat, inputRgb, CV_BGR2RGB);   ///> convert fro bgr to rgb.
    cv::Mat floatMat;                           ///> Mat to store float rgb image
    ///> Creates floatmat that stores values from 0-1
    inputRgb.convertTo(floatMat, CV_32FC3, 1/255.0);

    /**
     * Convert the three color layers to one continious where g follows r...
     */
    vector<cv::Mat> floatMatChannels(3);
    split(floatMat, floatMatChannels);
    vconcat(floatMatChannels, floatMat);
    return (float*) floatMat.data;
}

string Gandur::getLabel(const unsigned int &id) {
    if (id < l.classes) return string(classNames[id]);
    else return "error";
}

//convert to letterbox image.
cv::Mat Gandur::resizeLetterbox(const cv::Mat &input) {

    cv::Mat output;
    cv::Size dstSize(net->w, net->h);
    cv::Scalar bgcolor(128, 128,128);

    double h1 = dstSize.width * (input.rows/(double)input.cols);
    double w2 = dstSize.height * (input.cols/(double)input.rows);
    if (h1 <= dstSize.height) cv::resize(input, output, cv::Size(dstSize.width, h1));
    else cv::resize(input, output, cv::Size(w2, dstSize.height));
    int top = (dstSize.height-output.rows) / 2;
    int down = (dstSize.height-output.rows+1) / 2;
    int left = (dstSize.width - output.cols) / 2;
    int right = (dstSize.width - output.cols+1) / 2;
    cv::copyMakeBorder(output, output, top, down, left, right, cv::BORDER_CONSTANT, bgcolor);

    return output;
}

vector<string> Gandur::getClasses() {
    vector<string> v(classNames, classNames+l.classes);
    return v;
}

cv::Rect Gandur::ptoi(const int &width, const int &height, const box &b) {
    cv::Rect box;

    box.width = (int) (width * b.w);
    box.height = (int) (height * b.h);
    box.x = (int) (b.x * width - box.width / 2.);
    box.y = (int) (b.y * height - box.height / 2.);

    return box;
}


void Gandur::__Detect(float *inData, float thresh, float treeThresh) {
    /**
     * Darknet function to do the predictions
     * @param net network struct
     * @param inData image to do detections in
     */
    network_predict(net, inData);

    /**
     * Darknet function to get pox predictions from network
     * @param [in] l detection layer
     * @param [in] img.cols Image width
     * @param [in] img.rows Image height
     * @param [in] net->w network width
     * @param [in] net->h network height
     * @param [in] thresh detection threshold
     * @param [out] probs array with probabilities
     * @param [out] boxes array with detection boxes
     * @param [out] masks array with detection masks?
     * @param [in] only_objectness bool?
     * @param [in] map used in tree detecton?
     * @param [in] treeThresh detection threshold for tree type detection
     * @param [in] relative ?
     */
    get_region_boxes(l, img.cols, img.rows, net->w, net->h, thresh, probs, boxes, masks, 0, nullptr, treeThresh, 1);
    /**
     * Sort tree predictions
     */
    if (l.softmax_tree && nms)do_nms_obj(boxes, probs, nboxes, l.classes, nms);
        /**
         * NMS, supress different boxex in same prediction
         */
    else if (nms) do_nms_sort(boxes, probs, nboxes, l.classes, nms);

    /**
     * Loop through all prediction boxes.
     * Yolov2 does 5 prediction boxes per prediction cell, and gives a confidence
     * score for each class.
     *
     * Stores the most certain predictions in network
     */
    for (int i = 0; i < nboxes; ++i) {

        int class1 = max_index(probs[i], l.classes); ///> Get highest classid for box.
        float prob = probs[i][class1];               ///> Get the highest probability.

        /**
         * Add to detection vector if probability is higher than threshold
         */
        if (prob > thresh){
            Detection tmp;
            tmp.label= string(classNames[class1]);
            tmp.prob=prob;
            tmp.labelId=class1;
            tmp.box=ptoi(img.cols, img.rows, boxes[i]);
            detections.push_back(tmp);
        }
    }
}

/**
 * Cfg class implementation
 */
Cfg::Cfg() : thresh(0), treeThresh(0) {};

bool Cfg::check() {
    bool ret = true;
    if (!is_regular_file(names)) {
        cout << "No valid names file specified" << names << endl;
        ret = false;
    }
    /*
    if(!exists(names)){
        cout << "no names in namelsit file.."<<endl;
        ret = false;
    }
     */
    if (!is_regular_file(netCfg)) {
        cout << "No valid names file specified" << netCfg << endl;
        ret = false;
    }
    if (!is_regular_file(weights)) {
        cout << "No weights file specified" << weights << endl;
        ret = false;
    }
    return ret;
}

void Gandur::setThresh(const float &f) {
    cfg.thresh = f;
}

void Gandur::setTreeThresh(const float &f) {
    cfg.treeThresh = f;
}