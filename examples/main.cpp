#include "gandur.hpp"

using namespace cv;

Mat drawDetections(Mat img, std::vector<Detection> &dets) {

    for(Detection det : dets) {
        /*
        //Calculate degrees.
        float dpp = aov / sqrt(image.cols*image.cols + image.rows* image.rows) ;
        float degrees = o.rects[i].x+o.rects[i].width/2-image.cols/2;
        degrees *= dpp;
        */

        //DPRINTF("Label:%s prob: %00f \n\n", labels[objId].c_str(),probs[objId]);
        
        //Draw label
        char prob[5];
        sprintf(prob,"%1.2f",det.prob);

        Point posLabel=det.box.tl();  //top left

        putText(img, det.label, posLabel, FONT_HERSHEY_DUPLEX, 1, CV_RGB(0, 0, 0), 1, CV_AA);

        posLabel.x-=2;
        posLabel.y-=1;

        putText(img, det.label, posLabel, FONT_HERSHEY_DUPLEX, 1, CV_RGB(70, 250, 20), 1, CV_AA);   

        posLabel.x-=55;

        putText(img, prob, posLabel,FONT_HERSHEY_DUPLEX, 0.7,CV_RGB(0, 0, 0),1, CV_AA);
        
        posLabel.x-=1;
        posLabel.y-=1;

        putText(img, prob, posLabel, FONT_HERSHEY_DUPLEX, 0.7,CV_RGB(70, 200, 0),1, CV_AA);
        
        //Draw line from bottom 
        Point posCircle = Point(det.box.x+det.box.width/2, det.box.y+det.box.height);
        //Point posDegree = Point(det.box.x, det.box.y+det.box.height+14);
        
        // Draw sircle and line
        circle(img, posCircle, 4, cvScalar(50, 255, 200), 2);
        line(img, cvPoint(img.cols/2-1, img.rows), posCircle,cvScalar(0, 0, 0), 2);
        line(img, cvPoint(img.cols/2+1, img.rows),posCircle, CV_RGB(100, 200, 255), 1);
        // Show image and overlay using OpenCV
        
        rectangle(img, det.box,CV_RGB(100, 200, 255), 1, 8, 0);

        /*
        //Draw bearing from center
        char str[9]; 
        sprintf(str,"%3.2f", degrees);

        posDegree.x+=50;
        putText(img, str, posDegree,
                FONT_HERSHEY_DUPLEX, 1.3, CV_RGB(0, 0, 0), 2, CV_AA);
        posDegree.x-=2;
        posDegree.y-=1;         
        putText(img, str, posDegree,
                FONT_HERSHEY_DUPLEX, 1.3, CV_RGB(70, 250, 20), 1, CV_AA);
        */
    }
    return img;
}

bool ext(const std::string &file,const std::string &ext) {
    return file.substr(file.find_last_of('.') + 1) == ext;
}

int main(int argn, char** argv) {
    std::vector<Detection> dets;

    if (argn < 2) {
        cout << "./gandur mediafile\n";
        return -1;

    }
	std::string file = argv[1];
	auto *net = new Gandur();
	Mat image; 

	VideoCapture cap ( file );
    if( ! cap.isOpened () )  
    {
        EPRINTF("Could not load the AV file %s\n", file.c_str());
        return -1;
    }

    while(true) {
        dets.clear();
    	if(cap.read(image)) {

    	net->Detect(image);
    	dets = net->detections;

    	imshow("Gandur",drawDetections(image,dets));

        int k = waitKey(
            ext(file, "jpg") ||
            ext(file, "JPG") ||
            ext(file, "JPE") ||
            ext(file, "png") ? 0 : 10 );
		if(k==27) break;
	    }
	    else {
	    	EPRINTF("Cant read image / end of video..\n");
	    	break;
	    }

    } //detectionloop

	return 0;
}