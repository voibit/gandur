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
    nms = 0.4;  //TODO: figure out what this is....
    masks = nullptr;
    setlocale(LC_NUMERIC,"C");
    loadCfg("gandur.conf");
    loadVars();
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
    //DPRINTF("Setup: layers = %d, %d, %d\n", l.w, l.h, l.n);
    //DPRINTF("Image expected w,h = [%d][%d]!\n", net->w, net->h);
    size_t j;
    net = parse_network_cfg((char *) cfg.netCfg.c_str());
    l = net->layers[net->n - 1];
    //DPRINTF("Setup: net.n = %d\n", net->n);
    //DPRINTF("net.layers[0].batch = %d\n", net->layers[0].batch);
    loadWeights(cfg.weights);

    //speed up detection, other values are used only in training.
    set_batch_network(net, 1);
    net->subdivisions = 1;
    //Set detection layer
    //Set boxes;
    nboxes = (unsigned int) (l.w * l.h * l.n);
    boxes = (box *) calloc(nboxes, sizeof(box));
    probs = (float **) calloc(nboxes, sizeof(float *));

    if (l.coords > 4) {
        masks = (float **) calloc(nboxes, sizeof(float *));
        for (j = 0; j < nboxes; ++j) masks[j] = (float *) calloc(l.coords - 4, sizeof(float));
    }
    // Error exits
    if(!boxes || !probs) {
        EPRINTF("Error allocating boxes/probs, %p/%p !\n", boxes, probs);
        goto clean_exit;
    }
    for (j = 0; j < nboxes; ++j) {
        probs[j] = (float*)calloc(l.classes + 1, sizeof(float));
        if (!probs[j]) {
            EPRINTF("Error allocating probs[%d]!\n", j);
            goto clean_exit;
        }
    }
    DPRINTF("Setup: Done\n");
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
    img = inputMat.clone();
    detections.clear();

    if(inputMat.empty()) {
        EPRINTF("Error in inputImage! [bgr = %d, w = %d, h = %d]\n",
            !inputMat.data, inputMat.cols != net->w,
            inputMat.rows != net->h);
        return false;
    }

    if (inputMat.rows != net->h || inputMat.cols != net->w) {
        inputMat=resizeLetterbox(inputMat);
    }
    //Convert to darknet image format, and run detections.
    __Detect((float *) bgrToFloat(inputMat).data,
             thresh == 0 ? cfg.thresh : thresh,
             tree_thresh == 0 ? cfg.treeThresh : tree_thresh);
    return true;
}

cv::Mat Gandur::bgrToFloat(const cv::Mat &inputMat) {
    //Convert to rgb
    cv::Mat inputRgb;
    cvtColor(inputMat, inputRgb, CV_BGR2RGB);
    // Convert the bytes to float
    cv::Mat floatMat;
    inputRgb.convertTo(floatMat, CV_32FC3, 1/255.0);
    // Get the image to suit darknet
    vector<cv::Mat> floatMatChannels(3);
    split(floatMat, floatMatChannels);
    vconcat(floatMatChannels, floatMat);
    return floatMat;
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

//convert darknet box to cv rect & keep aspectratio.
cv::Rect Gandur::ptoi(const int &width, const int &height, const box &b) {
    cv::Rect box;

    box.width = (int) (width * b.w);
    box.height = (int) (height * b.h);
    box.x = (int) (b.x * width - box.width / 2.);
    box.y = (int) (b.y * height - box.height / 2.);

    return box;
}
/*
vector<Detection> Gandur::readLabels(path img) {

    vector<Detection> dets;
    int num_labels = 0;
    box_label *truth = read_boxes((char *)img.c_str(), &num_labels);

    for (size_t i = 0; i < num_labels; ++i) {
        box t = {truth[i].x, truth[i].y, truth[i].w, truth[i].h};
        dets.emplace_back(
                truth[i].id,
                classNames[truth[i].id],
                1,
                ptoi(t));
    }
    return dets;
}

*/
//////////////////////////////////////////////////////////////////
/// Private APIs
//////////////////////////////////////////////////////////////////
void Gandur::__Detect(float* inData, float thresh, float tree_thresh) {
    int i;

    network_predict(net, inData);
    get_region_boxes(l, img.cols, img.rows,net->w, net->h, thresh, probs, boxes, masks, 0, nullptr, tree_thresh,1);
    //Sorter boxer elns.
    DPRINTF("l.softmax_tree = %p, nms = %f\n", l.softmax_tree, nms);
    if (l.softmax_tree && nms)do_nms_obj(boxes, probs, nboxes, l.classes, nms);
    else if (nms) do_nms_sort(boxes, probs, nboxes, l.classes, nms);

    // Update object counts
    for (i = 0; i < nboxes; ++i) {

        int class1 = max_index(probs[i], l.classes);
        float prob = probs[i][class1];

        if (prob > thresh){
            Detection tmp;
            tmp.label= string(classNames[class1]);
            tmp.prob=prob;
            tmp.labelId=class1;
            tmp.box=ptoi(img.cols, img.rows, boxes[i]);
            detections.push_back(tmp);
        }
        /*
        //print all propabilities
        for(size_t j = 0; j < l.classes; j++) {
            
            if (probs[i][j] > 0.1) {
                printf("%s: %.0f%%\n", classNames[j], probs[i][j]*100);
            }
        }
        */
    }
}

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
