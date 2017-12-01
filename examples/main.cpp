/**
 *	@file main.cpp
 *	testprogram, https://github.com/voibit/gandur/examples/main.cpp
 *	@brief Program to test the gandur class
 *	@author Jan-Kristian Mathisen
 *	@author Joachim Lowzow
 */
#include "gandur.hpp"
#include <chrono>
#include <sstream>

using namespace cv;
#define FPS


string dtos(double d, int p=1) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(p) << d;
    return ss.str(); 
}
/**
 * Draws detection over the image.
 * @param img
 * @param dets
 * @return
 */
Mat drawDetections(Mat img, std::vector<Detection> &dets, double aov) {
    for (Detection det : dets) { ///> For each detection do;
        
        //Calculate degrees.
        double dpp = aov / sqrt(img.cols*img.cols + img.rows* img.rows) ;

        double degrees = det.box.x+det.box.width/2.;
        degrees -= img.cols/2;  ///> Converts to relative bearing
        degrees *= dpp;         ///> Convert from pixels

        Point posLabel = det.box.tl();  ///> top left position of box

        ///> Place shaddow.
        putText(img, det.label, posLabel, 2, 1, CV_RGB(0, 0, 0), 1, CV_AA);

        ///> Move
        posLabel.x-=2;
        posLabel.y-=1;

        ///> Place label in front of shadow.
        putText(img, det.label, posLabel, 2, 1, CV_RGB(70, 250, 20), 1, CV_AA);

        ///> Place probability shadow to the left of label.
        posLabel.x-=55;
        putText(img, dtos(det.prob,2), posLabel, 2, 0.7, CV_RGB(0, 0, 0), 1, CV_AA);

        ///> Draw probpability over shadow
        posLabel.x-=1;
        posLabel.y-=1;
        putText(img, dtos(det.prob,2), posLabel, 2, 0.7, CV_RGB(70, 200, 0), 1, CV_AA);

        ///> Draw line to detecton box.
        //Draw line from bottom 
        Point posCircle = Point(det.box.x + det.box.width / 2, det.box.y+det.box.height);
        // Draw sircle and line
        circle(img, posCircle, 4, cvScalar(50, 255, 200), 2);


        line(img, cvPoint(img.cols / 2 - 1, img.rows), posCircle, cvScalar(0, 0, 0),2);
        line(img, cvPoint(img.cols / 2 + 1, img.rows), posCircle, CV_RGB(100,200,255),1);

        ///> Draw rectancle around detection.
        rectangle(img, det.box, CV_RGB(100, 200, 255), 1, 8, 0);



        Size s = getTextSize(dtos(degrees),2,1.3,2,0);
        // Set color acording to degree
        Scalar color = degrees < 0? CV_RGB(255, 30, 30):CV_RGB(30, 255, 30); 

        //Center pos
        Point posDegree = Point(det.box.x+det.box.width/2, det.box.y+det.box.height/2);

        //adjust stb and port
        posDegree.x+= degrees < 0 ? -10: -s.width+4;

        // Add text with shaddow. 
        putText(img, dtos(degrees), posDegree,2, 1.3, 0, 3, CV_AA);
        posDegree.x-=2;
        posDegree.y-=1;
        putText(img, dtos(degrees), posDegree, 2, 1.32, color, 2, CV_AA);
    }
    return img;
}
/**
 * Get extension from filename
 * @param file
 * @param ext
 * @return file extension
 */
bool ext(const std::string &file, const std::string &ext) {
    return file.substr(file.find_last_of('.') + 1) == ext;
}

int main(int argn, char** argv) {


    const String keys =
        "{help h ?       |       | print this message  }"
        "{thresh         |       | detection threshold }"
        "{aov            | 60.   | angle of view diag  }"
        "{scale          | 1     | scale input         }"
        "{wait           | 5     | ms to show frame    }"
        "{fps            |       | show fps            }"
        "{@file          |       | mediafile to network}";

    CommandLineParser parser(argn, argv, keys);
    parser.about("Gandur test v1.0.0");

    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }
    if (argn < 2) {
        parser.printMessage();
        return -1;
    }

    string file = parser.get<String>(0);///> Filepath image, video, url etc
    vector<Detection> dets;             ///> Vector to store detections.
    auto *net = new Gandur();           ///> Initilize Yolov2
    Mat image;                          ///> Matrix to store frame / image
    float aov = 60;                     ///> Angle of view, camera /viceo.
    double scale = parser.get<double>("scale"); 
    int wait = parser.get<int>("wait"); 
    bool fps = parser.has("fps");

    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::milliseconds ms;
    typedef std::chrono::duration<float> fsec;
    auto t0 = Time::now();
    auto t1 = Time::now();

	VideoCapture cap ( file );
    if( ! cap.isOpened () )  {
        cout << "Could not load av-file: " << file << endl;
        return -1;
    }
    namedWindow("Gandur", WINDOW_NORMAL);// Create a window for display.
    if (parser.has("thresh")) {
        net->setThresh(parser.get<double>("thresh"));
    }
    if (parser.has("aov")) {
        aov = parser.get<double>("aov");
    }

    while(true) {

        dets.clear();
    	if(cap.read(image)) {


        if (scale !=1) {
            resize(image, image, Size(), scale, scale);
        }
        //
        //Rect box(0,0, 600,400);
        //image = image(box); 

    	net->Detect(image);

        dets=net->detections;

        Mat imdet = drawDetections(image,dets, aov);

        if (fps) {
            t1 = Time::now();
            fsec fs = t1 - t0;
            ms d = std::chrono::duration_cast<ms>(fs);
            double fps =1/(d.count()/1000.);
            putText(imdet, dtos(fps)+" fps", 
                Point(10,50), 2, 1, CV_RGB(0, 0, 0), 1, CV_AA);
            putText(imdet, dtos(fps)+" fps", 
                Point(10-2,50-1), 2, 1, CV_RGB(70, 250, 20), 1, CV_AA);
            t0 = Time::now();
        }

        imshow("Gandur",imdet);

        /**
         * Do not exit if filetype is image
         * Go to next frame if video.
         */
        
        int k = waitKey(
            ext(file, "jpg") ||
            ext(file, "JPG") ||
            ext(file, "JPE") ||
            ext(file, "png") ? 0 : wait );
		if(k==27) break;
	    }
	    else {
            cout << "End of file.. exiting.." << endl;
            break; ///> Breaks the while loop when no more frames to process.
        }

    } //end detection loop
    return 0;
}