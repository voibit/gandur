/**
 *	@file main.cpp
 *	testprogram, https://github.com/voibit/gandur/examples/main.cpp
 *	@brief Program to test the gandur class
 *	@author Jan-Kristian Mathisen
 *	@author Joachim Lowzow
 */
#include "gandur.hpp"
#include <chrono>

using namespace cv;

/**
 * Draws detection over the image.
 * @param img
 * @param dets
 * @return
 */
Mat drawDetections(Mat img, std::vector<Detection> &dets) {


    for (Detection det : dets) { ///> For each detection do;
        /*
        //Calculate degrees.
        float dpp = aov / sqrt(image.cols*image.cols + image.rows* image.rows) ;
        float degrees = o.rects[i].x+o.rects[i].width/2-image.cols/2;
        degrees *= dpp;
        */

        //TODO: Concert to c++.
        char prob[5];
        sprintf(prob, "%1.2f", det.prob);


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
        putText(img, prob, posLabel, 2, 0.7, CV_RGB(0, 0, 0), 1, CV_AA);

        ///> Draw probpability over shadow
        posLabel.x-=1;
        posLabel.y-=1;
        putText(img, prob, posLabel, 2, 0.7, CV_RGB(70, 200, 0), 1, CV_AA);

        ///> Draw line to detecton box.
        //Draw line from bottom 
        Point posCircle = Point(det.box.x + det.box.width / 2, det.box.y + det.box.height);
        //Point posDegree = Point(det.box.x, det.box.y+det.box.height+14);
        // Draw sircle and line
        circle(img, posCircle, 4, cvScalar(50, 255, 200), 2);
        line(img, cvPoint(img.cols / 2 - 1, img.rows), posCircle, cvScalar(0, 0, 0), 2);
        line(img, cvPoint(img.cols / 2 + 1, img.rows), posCircle, CV_RGB(100, 200, 255), 1);

        ///> Draw rectancle around detection.
        rectangle(img, det.box, CV_RGB(100, 200, 255), 1, 8, 0);

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

    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::milliseconds ms;
    typedef std::chrono::duration<float> fsec;
    auto t0 = Time::now();
    auto t1 = Time::now();


    const String keys =
            "{help h ?          |       | print this message        }"
                    "{thresh            |       | detection threshold       }"
                    "{@file             |       | mediafile to network      }";
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


    VideoCapture cap ( file );
    if( ! cap.isOpened () ) {
        cout << "Could not load av-file: " << file << endl;
        return -1;
    }

    if (parser.has("thresh")) {
        net->setThresh(parser.get<float>("thresh"));
    }

    while(true) {
        t0 = Time::now();
        dets.clear();
        if(cap.read(image)) {

            resize(image, image, Size(), 0.5, 0.5);

            net->Detect(image);
            dets = net->detections;
            t1 = Time::now();
            fsec fs = t1 - t0;
            ms d = std::chrono::duration_cast<ms>(fs);
            Mat imdet = drawDetections(image, dets);
            double fps = 1 / (d.count() / 1000.);
            ///> Add
            putText(imdet, std::to_string(fps) + " fps",
                    Point(10, 50), 2, 1, CV_RGB(0, 0, 0), 1, CV_AA);
            putText(imdet, std::to_string(fps) + " fps",
                    Point(10 - 2, 50 - 1), 2, 1, CV_RGB(70, 250, 20), 1, CV_AA);


            imshow("Gandur", imdet);
            net->Detect(image);     ///> Use Gandur to detect in image
            dets = net->detections; ///> get detections from Gandur.

            ///> Show image with detections
            imshow("Gandur", drawDetections(image, dets));

            /**
             * Do not exit if filetype is image
             * Go to next frame if video.
             */
            int k = waitKey(
                    ext(file, "jpg") ||
                    ext(file, "JPG") ||
                    ext(file, "JPE") ||
                    ext(file, "png") ? 0 : 10 );
            if(k==27) break;
        } else {
            cout << "End of file.. exiting.." << endl;
            break; ///> Breaks the while loop when no more frames to process.
        }

    } //end detection loop
    return 0;
}