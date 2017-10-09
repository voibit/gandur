#include "imgdet.hpp"

using namespace Imgdet; 

int main(int argc, char**argv){
    workPath=canonical(path(argc>1 ? argv[1] : "."));
    savePath=canonical(path(argc>2 ? argv[2] : "ok"));

    if (!is_directory(workPath)) {
        std::cout << workPath << " is not a directory. " << std::endl;
        return -1;  
    }
    if (!is_directory(savePath)) create_directory(savePath);
    
    net = new Gandur(); 
    imgs = getImgs(workPath); 
    count = imgs.size();
    classes = net->getClasses();

    show(0); 
    loopImgs();
	return 0; 
}


