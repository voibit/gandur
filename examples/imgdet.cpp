#include "gandur.hpp"
#include <iostream>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace boost::filesystem;


void saveTxt(const Mat &img, path p, const std::vector <Detection> &dets);

void classify(
    const Mat &orig,
    const std::vector<std::string> classes,
    const path &p,
    const path &fname);

bool isPic(path p) {
    bool ret = extension(p)==".jpg";
    ret |= extension(p)==".jpe";
    ret |= extension(p)==".jpeg";
    ret |= extension(p)==".png";
    ret |= extension(p)==".JPG";
    ret |= extension(p)==".PNG";
    return ret;
}

int main(int argc, char** argv) {

    path p(argc>1? argv[1] : ".");
    p=canonical(p);

    if (!is_directory(p / "ok")) create_directory(p / "ok");
    if (!is_directory(p / "tja")) create_directory(p / "tja");
    if (!is_directory(p / "nope")) create_directory(p / "nope");

    namedWindow("Gandur",WINDOW_AUTOSIZE);
    moveWindow("Gandur",0,0);
    Gandur *net = new Gandur();
    Mat image; 


    directory_iterator dirItr(p);
    for(auto &entry : dirItr) {

        path imgName=entry.path().filename();
        
        if (isPic(imgName)) {

            std::cout << std::endl << p/imgName << std::endl;
            std::cout << "[enter]=yes, [tab]=maybe, [s]=skip, [d]=delete, rest=no\n\n";
            image = imread( (p/imgName).string() );
             
            net->Detect(image,0.6, 0.5);

            imshow("Gandur",net->drawDetections());

            int k = waitKey(0);

            if(k==27) break;
            else if(k >47 && k < 48+9) k='c';

            switch(k) {
                case 9: //tab
                    std::cout << "Tja!";
                    copy_file(entry, p/"tja"/imgName,
                        copy_option::overwrite_if_exists);
                    saveTxt(image, p/"tja"/imgName, net->detections);
                    break;
                case 10: //enter
                case ' '://space
                    std::cout << "YEAAAH!";
                    copy_file(entry, p/"ok"/imgName,
                        copy_option::overwrite_if_exists);
                    saveTxt(image, p/"ok"/imgName, net->detections);
                    break;
                case 'd':
                    std::cout << "Are you shure you want to delete: ";
                    std::cout << imgName << "? (y/N)\n"; 
                    if (waitKey(0)=='y') {
                        remove(p/imgName);
                        std::cout << imgName <<" deleted.."; 
                    }
                    break;
                case 'c':
                    classify(image,net->getClasses(), p, imgName);
                    break;
                case '<':
                    std::cout << "skipping " << imgName << "..";
                    break;
                default: 
                    std::cout << "NONO!! saing in nope,  you pressed "<< k << std::endl;
                    copy_file(entry, p/"nope"/imgName,
                        copy_option::overwrite_if_exists);
                    break;
            }
            std::cout << std::endl;
        }
    }
    delete net;
    net = 0;
	return 0;
}

void saveTxt(const Mat &img, path p, const std::vector<Detection> &dets) {
                    
    fstream file(p.replace_extension(".txt"), std::ios::out);

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

void classify(
    const Mat &orig,
    const std::vector<std::string> classes,
    const path &p,
    const path &fname) {

    Mat img=orig.clone();
    std::cout << "Manual classifer classify\n";
    std::vector<Detection> dets; 

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
