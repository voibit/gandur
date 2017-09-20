#include "gandur.hpp"
#include <iostream>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace boost::filesystem;

void classify(
    const Mat &orig,
    const std::vector<std::string> classes,
    const path &p,
    const path &fname) {

    Mat img=orig.clone();
    std::cout << "Manual classifer classify\n";
    std::vector<Rect> boxes;
    std::vector<int> ids;

    while(1) {
        std::cout << "press 0-9 for class id, r reset, s save, c cancel.  \n";
        int k = waitKey(0);

        //TODO: Add another way to select classes..
        if (k=='q' || k== 27) break;
        else if (k=='r') {
            boxes.clear();
            ids.clear();
            img=orig.clone();
            imshow("Gandur",img);

        }
        else if (k >47 && k < 48+classes.size()) {
            int id=k-48;
            std::cout << " id: "<< id << classes[id] << std::endl;
            
            rectangle(img, Rect(0,0,img.cols,26),CV_RGB(0, 0, 0), CV_FILLED, 8, 0);
            putText(img, classes[id], Point(0,20), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 230, 230), 1, CV_AA);
            
            imshow("Gandur",img);
            Rect test = selectROI("Gandur",img);
            if (test.width!=0 && test.height!=0) {
                putText(img, classes[id], Point(test.x,test.y), FONT_HERSHEY_DUPLEX, 1, CV_RGB(0, 0, 0), 1, CV_AA);
                rectangle(img, test,CV_RGB(100, 200, 255), 1, 8, 0);
                imshow("Gandur",img);

                boxes.push_back(test);
                ids.push_back(id);

            }
        }
        else if(k=='s') {
            path newFile = p/"ok"/fname;
            copy_file(p/fname, newFile, copy_option::overwrite_if_exists);
            fstream file(newFile.replace_extension(".txt"), std::ios::out);

            for (size_t id = 0; id < ids.size(); id++) {
                if (id > 0) file << std::endl;
                file << ids[id]; 
                file << " " << (boxes[id].x + boxes[id].width/2.)/img.cols;
                file << " " << (boxes[id].y + boxes[id].height/2.)/img.rows;
                file << " " << boxes[id].width / (float)img.cols;
                file << " " << boxes[id].height / (float)img.rows;  
            }
            std::cout << newFile << " saved!\n";
            file.close();
            break;
        } 
    }
}

int main(int argc, char** argv) {

    path p(argc>1? argv[1] : ".");

    if (!is_directory(p / "ok")) create_directory(p / "ok");
    if (!is_directory(p / "tja")) create_directory(p / "tja");
    if (!is_directory(p / "nope")) create_directory(p / "nope");

    namedWindow("Gandur",WINDOW_AUTOSIZE);
    moveWindow("Gandur",0,0);
    Gandur *net = new Gandur();
    Mat image; 

    for(auto& entry : directory_iterator(p)) {
        path newPath = canonical(p);
        if (extension(entry)==".jpg" || extension(entry)==".png") {

            path imgName=entry.path().filename();

            std::cout << std::endl << newPath/imgName << std::endl;
            std::cout << "[enter]=yes, [tab]=maybe, [s]=skip, [d]=delete, rest=no\n\n";
            image = imread( (newPath/imgName).string() );
             
            net->Detect(image,0.6, 0.5);

            imshow("Gandur",net->drawDetections());

            char k = waitKey(0);

            if(k==27) break;
            else if(k==char(10) || k==char(9) || k==' ') { //char(10) = enter 9=tab

                if (k==char(10) || k==' ') {
                    std::cout << "YEAAAH!";
                    newPath/="ok";
                }
                else {
                    std::cout << "Tja!";
                    newPath/="tja";
                }
                newPath/=imgName;

                copy_file(entry, newPath, copy_option::overwrite_if_exists);
                fstream file(newPath.replace_extension(".txt"), std::ios::out);

                float x,y, w, h;

                for (auto det : net->detections) {

                    w = det.box.width / float(image.cols);
                    h = det.box.height / float(image.rows);
                    x = (det.box.x + det.box.width/2.) / image.cols;
                    y = (det.box.y + det.box.height/2.) / image.rows;
                    
                    file << net->getLabelId(det.label);
                    file << " " << x << " " << y << " " << w << " " << h << std::endl;
                }
                file.close();
            }
            else if (k=='d') {
                std::cout << "Are you shure you want to delete: " << imgName << "? (y/N)";  
                char l = waitKey(0); 
                if (l=='y') {
                    remove(newPath/imgName);
                    std::cout << imgName <<" deleted.."; 
                } 
            }
            else if (k=='c') {
                classify(image,net->getClasses(), newPath, imgName);
            }
            else if (k=='s') {
                std::cout << "skipping " << imgName << "..";  
            }
            else {
                std::cout << "NONO!!";
                copy_file(entry, newPath/"nope"/imgName, copy_option::overwrite_if_exists);
            }
            std::cout << std::endl;
        }
    }
    delete net;
    net = 0;
	return 0;
}