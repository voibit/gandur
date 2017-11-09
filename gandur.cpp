/*************************************************************************
 * Gandur, https://github.com/voibit/gandu                               *
 * C++ API for Yolo v2                                                   *                                                                   *
 * Forked from arapaho, https://github.com/prabindh/darknet              *
 *************************************************************************/
#include "gandur.hpp"

Gandur::Gandur() {
    boxes = nullptr;
    probs = nullptr;
    classNames = nullptr;
    l = {};
    net = {};
    nms = 0.4;  //TODO: figure out what this is.... 
    thresh = 0;
    masks = nullptr;
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
    masks = nullptr;
    boxes = nullptr;
    probs = nullptr;
    classNames = nullptr;
}
    
bool Gandur::Setup() {
    if(!exists("gandur.conf")) {
        cout << "No config file found, exiting\n";
        return false;
    }

    configFile=canonical("gandur.conf"); //get full path to config file
    options = read_data_cfg((char *)configFile.string().c_str());
    char *nameListFile = option_find_str(options, (char*)"names", (char*)"data/names.list");
    networkFile = option_find_str(options, (char*)"networkcfg", (char*)"data/sea.cfg");
    char *weightsFile = option_find_str(options, (char*)"weights", (char*)"data/sea.weights");
    thresh = option_find_float(options, (char*)"thresh", 0.5);

    if(!nameListFile){
        DPRINTF("No valid nameList file specified in options file [%s]!\n", configFile.string().c_str());
        return false;
    }
    classNames = get_labels(nameListFile);
    if(!classNames){
        DPRINTF("No valid class names specified in nameList file [%s]!\n", nameListFile);
        return false;
    }

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

    net->subdivisions = 1;
    //Set detection layer
    l = net->layers[net->n-1];

    DPRINTF("Setup: layers = %d, %d, %d\n", l.w, l.h, l.n);
    DPRINTF("Image expected w,h = [%d][%d]!\n", net->w, net->h);
    boxes = (box*)calloc(l.w*l.h*l.n, sizeof(box));
    probs = (float**)calloc(l.w*l.h*l.n, sizeof(float *));

    size_t j;

    if (l.coords > 4){
        masks = (float**)calloc(l.w*l.h*l.n, sizeof(float*));
        for(j = 0; j < l.w*l.h*l.n; ++j) masks[j] = (float*)calloc(l.coords-4, sizeof(float));
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
    boxes = nullptr;
    probs = nullptr;
    return false;
}

bool Gandur::Detect(cv::Mat inputMat,float thrs, float tree_thresh){
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

    __Detect((float*)bgrToFloat(inputMat).data, thrs > 0 ? thrs:thresh, tree_thresh);
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

int Gandur::getLabelId(const string &name) {

    for (size_t i = 0; i < l.classes; i++) {
        if (string(classNames[i]) == name) return i;         
    }
    return -1; 
}
string Gandur::getLabel(const unsigned int &id) {
    if (id < l.classes) {
        return string(classNames[id]);
    }
    else return "error";
}

//convert to letterbox image.
cv::Mat Gandur::resizeLetterbox(const cv::Mat &input) {

    cv::Mat output;
    cv::Size dstSize(net->w, net->h);
    cv::Scalar bgcolor(128, 128,128);

    double h1 = dstSize.width * (input.rows/(double)input.cols);
    double w2 = dstSize.height * (input.cols/(double)input.rows);
    if( h1 <= dstSize.height) {
        cv::resize( input, output, cv::Size(dstSize.width, h1));
    } else {
        cv::resize( input, output, cv::Size(w2, dstSize.height));
    }
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

bool Gandur::validate(path backupdir, path validfile) {

    string cfgname=path(string(networkFile)).filename().replace_extension("").string();
    float iou_thresh = option_find_float(options, (char*)"iou-thresh", 0.5);

    if (validfile == "") {
        validfile = string(option_find_str(options, (char *) "valid", (char *) "../darknet/valid.txt"));
    }

    if(!exists(validfile)) {
        cout << "Valid file not found. please specyfy in conf\n"; 
        return false;
    }

    if (backupdir == "") {
        backupdir = string(option_find_str(options, (char *) "backup", (char *) "/backup/"));
    }
    if(!exists(backupdir)) {
        cout << "backup dir not found. please specify in conf\n";
        return false;
    }
    vector<path> weights;
    vector<path> imgs; 
    /*
    //fill weights vector; 
    for(auto &entry : directory_iterator(backupdir)) {
        path filename=entry.path().filename();
        if (filename.extension()==".weights") {
        
            //check if weights name is the same as cfg name
            string name =filename.string();
            if (name.substr(0, name.find('_')) == cfgname) {
                weights.push_back( (backupdir/filename).string() );
            }    
        }
    }
    sort(weights.begin(), weights.end());
    */

    if (is_regular_file(backupdir)) {
        weights.push_back(backupdir);
    } else {
        for (int i = 1000; i < 200000; i += 1000) {
            path wname = cfgname + "_" + std::to_string(i) + ".weights";
            if (exists(backupdir / wname)) {
                weights.push_back(backupdir / wname);
            }
        }
    }

    //fill image vector
    ifstream file(validfile);
    string fname;
    while (std::getline(file, fname)) {
        imgs.push_back(path(fname));
    }
    file.close();

    ofstream ofile(path(cfgname+".csv"));
    
    char delim =',';

    ofile << "Weight" << delim<< "IOU" << delim << "mAP"+std::to_string((int)(thresh*100))<< delim << "mAP 50"<< delim << "mAP 70"<< delim << "mAP 90" << delim << "wrong\n";  
    cout << "Weight\tIOU\tmAP\n";  

    for (path weight : weights) {

        int wrong = 0;
        int total = 0;
        int correct = 0;
        float avg_iou = 0;
        int c50=0;
        int c70=0;
        int c90=0;
     
        load_weights(net, (char*)weight.string().c_str());

        ofstream ofilee(weight.replace_extension("error.csv"));
        ofilee << "file" << delim <<"boxnr" <<delim << "classification[true-proposed]"<< endl;

        for (path p : imgs) {
            img = cv::imread(p.string());
            cv::Mat sized=resizeLetterbox(img);
            
            //Read box from labelfile
            p.replace_extension(".txt");
            int num_labels = 0;
            box_label *truth = read_boxes((char *)p.string().c_str(), &num_labels);

            //Predict probs and boxes from weights
            network_predict(net, (float*)bgrToFloat(sized).data );
            get_region_boxes(l, img.cols, img.rows, net->w, net->h, thresh, probs, boxes, masks, 0, nullptr, .5, 1);

            if (l.softmax_tree && nms) do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
            else if (nms) do_nms_sort(boxes, probs, l.w*l.h*l.n, l.classes, nms);

            //Loop every object in image 
            for (int j = 0; j < num_labels; ++j) {

            ++total;
            box t = {truth[j].x, truth[j].y, truth[j].w, truth[j].h};

            std::stringstream  ss;

            for(int k = 0; k < l.w*l.h*l.n; ++k){

                float iou = box_iou(boxes[k], t);
                float prob = probs[k][truth[j].id];
                int maxindex=max_index(probs[k], l.classes);
                float maxprob =probs[k][maxindex];

                if(maxprob > thresh && iou > iou_thresh){  //passer på at boksen stemmer overens med orginalen
                    //Check if other class for same box has a higher propability.
                    if (maxindex != truth[j].id) {
                        ss.clear();
                        ++wrong;
                        ofilee << p <<delim << j << delim <<classNames[truth[j].id]<<" "<<prob*100<<"%"<<classNames[maxindex]<<" "<<maxprob*100<<"%\n";
                    }
                    else {
                        ++correct;
                        avg_iou+=iou;
                    }
                    if(prob > .5){
                        ++c50;
                        if(prob > .7){
                            ++c70;
                            if(prob > .9) ++c90;
                        }
                    }
                }
                /*extra detections. 
                else if (maxprob > thresh) {
                
                }
                */
            }
        }//NUM LABELS LOOP
    } //img loop
    ofile << weight.filename() << delim<< avg_iou*100/total << delim<< 100.*correct/total<< delim;
    ofile << 100.*c50/total <<delim << 100.*c70/total<<delim<< 100.*c90/total  <<  delim << 100.*wrong/total << std::endl;
    cout << weight.filename() << "\t Avg iou:"<< avg_iou*100/total << "\t mAP:"<< 100.*correct/total << std::endl;
    
    ofilee.close();
    } // weights loop
    ofile.close();
}

//////////////////////////////////////////////////////////////////
/// Private APIs
//////////////////////////////////////////////////////////////////
void Gandur::__Detect(float* inData, float thresh, float tree_thresh) {
    int i;
    network_predict(net, inData);
    get_region_boxes(l, img.cols, img.rows,net->w, net->h, thresh, probs, boxes, masks, 0, nullptr, tree_thresh,1);

    //Sorter boxer elns.
    DPRINTF("l.softmax_tree = %p, nms = %f\n", l.softmax_tree, nms);
    if (l.softmax_tree && nms)do_nms_obj(boxes, probs, l.w * l.h * l.n, l.classes, nms);
    else if (nms) do_nms_sort(boxes, probs, l.w * l.h * l.n, l.classes, nms);

    // Update object counts
    for (i = 0; i < (l.w*l.h*l.n); ++i){

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
//convert darknet box to cv rect & keep aspectratio. 
cv::Rect Gandur::ptoi(const int &width, const int &height, const box &b) {
            cv::Rect box;

            box.width = (int)(width * b.w);
            box.height = (int)(height * b.h);
            box.x = (int)(b.x*width - box.width/2.);
            box.y = (int)(b.y*height- box.height/2.);
        
    return box;
}