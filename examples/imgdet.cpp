#include "gandur.hpp"
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace boost::filesystem;

int main(int argc, char** argv) {

    path p(argc>1? argv[1] : ".");

    if (!is_directory(p / "ok")) create_directory(p / "ok");
    if (!is_directory(p / "tja")) create_directory(p / "tja");
    if (!is_directory(p / "nope")) create_directory(p / "nope");

    Gandur *net = new Gandur();
    Mat image; 

    for(auto& entry : directory_iterator(p)) {
        path newPath = canonical(p);
        if (extension(entry)==".jpg" || extension(entry)==".png") {

            path imgName=entry.path().filename();

            std::cout << std::endl << newPath/imgName << std::endl;
            std::cout << "[enter]=yes, [tab]=maybe, [s]=skip, [d]=delete, rest=no\n";
            std::cout << newPath/imgName << std::endl;  
            image = imread( (newPath/imgName).string() );
             
            net->Detect(image,0.6, 0.5);

            imshow("Gandur",net->drawDetections());

            char k = waitKey(0);

            if(k==27) break;
            else if(k==char(10) || k==char(9)) { //char(10) = enter 9=tab

                if (k==char(10)) {
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