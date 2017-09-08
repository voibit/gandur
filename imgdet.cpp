#include "gandur.hpp"

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

using namespace cv;
using namespace boost::filesystem;

int main(int argc, char** argv) {

    path p(argc>1? argv[1] : ".");

    if (!is_directory(p / "ok")) create_directory(p / "ok");
    if (!is_directory(p / "tja")) create_directory(p / "tja");
    if (!is_directory(p / "nope")) create_directory(p / "nope");

    Gandur *net = new Gandur();
    Mat image; 

    std::cout << p << " is a directory containing:\n";

    for(auto& entry : boost::make_iterator_range(directory_iterator(p), {})) {
        std::cout << entry << "\n";
        if (extension(entry)==".jpg" || extension(entry)==".png") {

            path imgPath = canonical(entry);
            path name = imgPath.leaf();

            image = imread( imgPath.string() );
            net->Detect(image,0.6, 0.5);

            imshow("Gandur",net->drawDetections());

            char k = waitKey(0);
            if(k==27) break;
            else if(k=='y') {
                std::cout << "YEAAAH! \n";
                path newPath = imgPath.remove_leaf()/"ok"/name;
                copy_file(entry,  newPath, copy_option::overwrite_if_exists);
                
                fstream file(newPath.replace_extension(".txt"), std::ios::out);

                float x,y, w, h;

                for (auto det : net->detections) {

                    w = det.box.width / float(image.cols);
                    h = det.box.height / float(image.rows);
                    x = (det.box.x + det.box.width/2) / float(image.cols);
                    y = (det.box.y + det.box.height/2) / float(image.rows);
                    
                    file << net->getLabelId(det.label);
                    file << " " << x << " " << y << " " << w << " " << h << std::endl;

                }
                file.close();
            }

            else if(k=='f') {
                std::cout << "NESTEN!! \n";
            }
            else {
                std::cout << "NONO!!\n";
            }
        }
    }
    if(net) delete net;
    net = 0;

	return 0;
}