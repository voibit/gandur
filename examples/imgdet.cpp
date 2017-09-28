#include "gandur.hpp"
#include <iostream>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace boost::filesystem;
using std::vector;
using std::string; 


bool isPic(path p);
void process(vector<path> &imgs,Gandur *net);
void classify(const Mat &orig,const vector<string> classes,const path &p,const path &fname);
void saveTxt(const Mat &img, path p, const vector <Detection> &dets);



int main(int argc, char** argv) {

    path p(argc>1? argv[1] : ".");
    p=canonical(p);

    vector<path> imgs; 

    if (!is_directory(p / "ok")) create_directory(p / "ok");

    namedWindow("Gandur",WINDOW_AUTOSIZE);
    moveWindow("Gandur",0,0);
    Gandur *net = new Gandur();

    for(auto &entry : directory_iterator(p)) {
        path imgName=entry.path().filename();
        if (isPic(imgName)) {
            imgs.push_back(p/imgName);
        }
        process(imgs, net);
    }
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
    while (i<imgs.size()) {

        path p, imgName, imgPath, txt; 
        p = imgs[i].parent_path();
        imgName = imgs[i].filename();
        imgPath = txt = p/"ok"/imgName;
        txt.replace_extension(".txt");

        std::cout << std::endl << imgs[i] << std::endl;
        std::cout << "[enter]=yes, [s]=skip, [d]=delete, rest=no\n\n";
        image = imread( imgs[i].string() );
         
        net->Detect(image,0.6, 0.5);

        if (exists(imgPath) && exists(txt)) {

        }
        else {
            imshow("Gandur",net->drawDetections());
        }
        

        int k = waitKey(0);

        std::cout << k; 

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
            copy_file(imgs[i], p/"ok"/imgName,
                copy_option::overwrite_if_exists);
            saveTxt(image, txt, net->detections);
            break;
            case 'd':
            std::cout << "Are you shure you want to delete: ";
            std::cout << imgName << "? (y/N)\n"; 
            if (waitKey(0)=='y') {
                remove(imgs[i]);
                std::cout << imgName <<" deleted.."; 
            }
            break;
            case 'c':
            case 9: //tab
            classify(image,net->getClasses(), p, imgName);
            break;
            default: 
            i++;
            break;
        }
        std::cout << std::endl;
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
    
    float id, x, y, w, h;
    while (getline(file, line)) {
        Detection det; 

        stringstream ss(line);

        ss << id, x, y, w, h; 
    }


}

void classify(
    const Mat &orig,
    const vector<string> classes,
    const path &p,
    const path &fname) {

    Mat img=orig.clone();
    std::cout << "Manual classifer classify\n";
    vector<Detection> dets; 

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
            std::cout << " id: "<< id << classes[id] << std::endl;
            
            rectangle(img, Rect(0,0,img.cols,26),CV_RGB(0, 0, 0), CV_FILLED, 8, 0);
            putText(img, classes[id], Point(0,20), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 230, 230), 1, CV_AA);
            
            imshow("Gandur",img);
            Rect box = selectROI("Gandur",img);

            if (box.width!=0 && box.height!=0) {
                Detection det;

                det.box = box;
                det.labelId = id; 
                det.label = classes[id];
                dets.push_back(det);

                //TODO: loop thorug dets insted of overwriting current image
                putText(img, classes[id], Point(box.x,box.y), FONT_HERSHEY_DUPLEX, 1, CV_RGB(0, 0, 0), 1, CV_AA);
                rectangle(img, box,CV_RGB(100, 200, 255), 1, 8, 0);
                imshow("Gandur",img);
            }
        }
        else if(k=='s') {
            path newFile = p/"ok"/fname;
            copy_file(p/fname, newFile, copy_option::overwrite_if_exists);
            saveTxt(img, newFile, dets);

            break;
        } 
    }
}
