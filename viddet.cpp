#include "gandur.hpp"
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <opencv2/core/utility.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <chrono>
#include "Http_server.hpp"
#include <vector> 
#include <fstream>


using namespace cv;

bool ext(const std::string file, std::string ext) {
	if (file.substr(file.find_last_of(".") + 1) == ext) {
		return true;
	}
	else return false;
}

int main(int argn, char** argv) {

	std::string file = argv[1];
	Gandur *net = new Gandur();
	Mat image; 


	VideoCapture cap ( file );
    if( ! cap.isOpened () )  
    {
        EPRINTF("Could not load the AV file %s\n", file.c_str());
        if(net) delete net;
        net = 0;
        return -1;
    }

    int i=0;

    while(1) {

    
    	if(cap.read(image)) {

    	net->Detect(image,0.6, 0.5);
    	
    	imshow("Gandur",net->drawDetections());

        char k = waitKey(0);
		if(k==27) break;
        else if(k=='y') {
            
            


        }
        else {
            

        }

	    }
	    else {
	    	EPRINTF("Cant read image / end of video..\n");
	    	break;
	    }

    } //detectionloop

    if(net) delete net;
    net = 0;

	return 0;
}

