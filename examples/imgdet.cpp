#include "gandur.hpp"
#include <iostream>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace boost::filesystem;
using std::vector;
using std::string; 
bool isPic(path p);
void process(vector<path> &imgs,Gandur *net);
void classify(Mat img, const vector<string> classes,const path &p,const path &fname, vector<Detection> &dets);
void saveTxt(const Mat &img, path p, const vector <Detection> &dets);
vector<Detection> readTxt(Mat &img, path &p, Gandur *net);
void draw(Mat &img, vector<Detection> dets);
void banner(Mat &img, Rect box, Scalar rgba=Scalar(0,0,0,0.7) );

path savedir;

int main(int argc, char** argv) {

    path p(argc>1? argv[1] : ".");
    p=canonical(p);
    
    if (argc > 2 && is_directory(argv[2])) savedir=canonical(argv[2]);
    else {
        savedir=p/"ok";
        if (!is_directory(savedir)) create_directory(savedir);
    }

    vector<path> imgs; 

    namedWindow("Gandur",WINDOW_AUTOSIZE);
    moveWindow("Gandur",0,0);
    Gandur *net = new Gandur();

    for(auto &entry : directory_iterator(p)) {
        path imgName=entry.path().filename();
        if (isPic(imgName)) {
            imgs.push_back(p/imgName);
        }
    }
    process(imgs, net);
    delete net;
    net = 0;
	return 0;
}

bool isPic(path p) {
    bool ret = extension(p)==".jpg";
    ret |= extension(p)==".jpe";
    ret |= extension(p)==".jpeg";
    ret |= extension(p)==".png";
    ret |= extension(p)==".JPG";
    ret |= extension(p)==".PNG";
    return ret;
}

void process(vector<path> &imgs,Gandur *net) {
    Mat image;
    size_t i = 0;

    std::cout << imgs.size() << std::endl ;
    while (i<imgs.size()) {

        path p, imgName, imgPath, txt; 
        p = imgs[i].parent_path();
        imgName = imgs[i].filename();
        imgPath = txt = savedir/imgName;
        txt.replace_extension(".txt");

        std::cout << imgs[i] << std::endl;
        std::cout << "imgname: " << imgName << std::endl;
        std::cout << "imgpath: " << imgPath << std::endl;
        std::cout << "txtpath: " << txt << std::endl;

        std::cout << "[enter]=yes, [s]=skip, [d]=delete, rest=no\n\n";
        image = imread( imgs[i].string() );
 
        banner(image,Rect(0,0,image.cols,26)); 
        string message=std::to_string(i)+" "+imgName.string();
        putText(image, message , Point(0,20), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(30, 230, 150), 1, CV_AA);
       
        vector<Detection> dets; 

        if (exists(imgPath) && exists(txt)) {
            message="already labeled.";
            std::cout << message << "\n";
            Size textSize = getTextSize(message, FONT_HERSHEY_DUPLEX , 0.5, 1,0);
            putText(image, message, Point(image.cols-textSize.width,20), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(30, 230, 150), 1, CV_AA);
            dets=readTxt(image,txt,net);
            draw(image, dets);
        }
        else {
            net->Detect(image,0.6, 0.5);
            dets = net->detections;
            draw(image, dets);
        }

        int k = waitKey(0);

        if(k==27) break;
        else if(k >47 && k < 48+9) k='c';

        switch(k) {
            case 81: //left
            i--; 
            break; 
            case 83: //right
            i++; 
            break;  
            case 10: //enter
            case ' '://space
            std::cout << "YEAAAH!";
            copy_file(imgs[i], savedir/imgName,
                copy_option::overwrite_if_exists);
            saveTxt(image, txt, net->detections);
            i++; 
            break;
            case 'd':
            std::cout << "Are you shure you want to delete: ";
            std::cout << imgName << "? (y/N)\n"; 
            if (waitKey(0)=='y') {
                remove(imgs[i]);
                imgs.erase(imgs.begin()+i);
                std::cout << imgName <<" deleted.."; 
            }
            break;
            case 'c':
            case 9: //tab
            classify(image, net->getClasses(), p, imgName, dets);
            i++;
            break;
            default: 
            i++;
            break;
        }
        std::cout << std::endl;
    }
}
void classify(
    Mat img,
    const vector<string> classes,
    const path &p,
    const path &fname,
    vector<Detection> &dets) {

    Mat orig=imread((p/fname).string());
    std::cout << "Manual classifer classify\n"; 

    while(1) {
        std::cout << "press 0-9 for class id, r reset, s save, c cancel.  \n";
        int k = waitKey(0);
        if (k=='q' || k== 27) break;

        //reset classification. 
        else if (k=='r') {
            dets.clear();
            img=orig.clone();
            imshow("Gandur",img);
        }
        else if (k >47 && k < 48+classes.size()) {
            int id=k-48;
            std::cout << "id: "<< id << " " << classes[id] << std::endl;
            
            
            Size textSize = getTextSize(classes[id], FONT_HERSHEY_DUPLEX , 0.5, 1,0);
            int middle = img.cols/2-textSize.width/2;
            banner(img, Rect(middle-20,0,textSize.width+40,26), Scalar(0,0,0));
            putText(img, classes[id], Point(middle,20), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(0, 230, 230), 1, CV_AA);
            
            imshow("Gandur",img);
            Rect box = selectROI("Gandur",img);

            if (box.width!=0 && box.height!=0) {
                Detection det;
                det.box = box;
                det.labelId = id; 
                det.label = classes[id];
                dets.push_back(det);
                draw(img, dets);
            }
        }
        else if(k=='s') {
            path newFile = savedir/fname;
            copy_file(p/fname, newFile, copy_option::overwrite_if_exists);
            saveTxt(img, newFile.replace_extension(".txt"), dets);
            break;
        } 
    }
}

void saveTxt(const Mat &img, path p, const vector<Detection> &dets) {
                    
    ofstream file(p);

    for (size_t i = 0; i<dets.size();i++) {
        if (i > 0) file << std::endl;
        file << dets[i].labelId;
        file << " " << (dets[i].box.x + dets[i].box.width/2.)/img.cols;
        file << " " << (dets[i].box.y + dets[i].box.height/2.)/img.rows;
        file << " " << dets[i].box.width / (float)img.cols;
        file << " " << dets[i].box.height / (float)img.rows;  
    }
    file.close();
    std::cout << p << " saved!\n";
}

vector<Detection> readTxt(Mat &img, path &p, Gandur *net) {
    vector<Detection> dets; 
    string line;
    ifstream file(p);
    
    float x, y, w, h;
    int id, X,Y, W, H; 
    while (getline(file, line)) {
        Detection det; 
        std::istringstream ss(line);

        ss >> id >> x >> y >> w >> h;

        W = w * img.cols;
        H = h * img.rows;
        X = x*img.cols -W/2;
        Y = y*img.rows - H/2;

        det.box=Rect(X,Y,W,H); 
        det.labelId=id;
        det.label=net->getLabel(id);
        det.prob=1;
        dets.push_back(det); 
    }
    return dets; 
}

void banner(Mat &img, Rect box, Scalar rgba) {

        double alpha = rgba[3] == 0 ? 1: rgba[3];
        Scalar rgb = CV_RGB(rgba[0],rgba[1],rgba[2]);
        Mat roi = img(box);
        Mat color(roi.size(),CV_8UC3, rgb);
        addWeighted(color, alpha, roi, 1.0 - alpha , 0.0, roi); 
}

void draw(Mat &img, vector<Detection> dets) {

    for (auto det : dets) {
        Point txtpos(det.box.x+2,det.box.y+1); 
        putText(img, det.label, txtpos, FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 0, 0), 1, CV_AA);
        txtpos.x-=2;txtpos.y-=1; 
        putText(img, det.label, txtpos, FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 255, 50), 1, CV_AA);
        rectangle(img, det.box,CV_RGB(100, 200, 255), 1, 8, 0);
    }
    imshow("Gandur",img);
}