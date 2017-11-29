/**
 *	@file videt.cpp
 *	videt, https://github.com/voibit/gandur/examples/videt.cpp
 *	@brief program to dump frames from video
 *	@author Jan-Kristian Mathisen
 *	@author Joachim Lowzow
 */
//TODO: Network is disabled.

//#include "gandur.hpp"
#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace boost::filesystem;
using std::cout;
using std::endl;

int seeker = 0;
bool seek = false;      ///> Used to seek video
bool moveSeek = false;  ///> Used not to seek video when moving seeker

/**
 * Set control varaible, if seeking.
 */
static void setSeek(int, void* )
{
    if (!moveSeek) seek = true;
    else moveSeek = false;
}

int main(int argc, char** argv) {

    ///> Get mediafile from commandline argument
    path p(argc > 1 ? argv[1] : "mediafile.mp4");

    path filename = p.filename();   ///> Use onlu filename,
    p = canonical(p).parent_path(); ///> Get full path of file.

    ///> Make save directory if it does not exist
    if (!is_directory(p / "ok")) create_directory(p / "ok");

    Mat srcImg, image;
    std::vector<Rect> boxes;
    std::vector<int> compression_params;
    compression_params.push_back(IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);

    ///> Open video file or stream
    VideoCapture cap((p / filename).string());
    int n=0;
    if (!cap.isOpened()   ///> Something went wrong
    {
        std::cout << "Could not open file: " << p << std::endl;
        return -1;
    }
    namedWindow("Gandur", WINDOW_AUTOSIZE);
    moveWindow("Gandur", 0, 0);

    //Gandur *net = new Gandur();
    filename.replace_extension("");
    std::string fname = filename.string();
    p/="ok";
    //get highest number in folder
    int i=0;

    ///> Find highest index of saved images
    for(auto& entry : directory_iterator(p)) {
        path readfile=entry.path();
        readfile.replace_extension("");

        /**
         * do some string magic to determin highest number of current filename
         * dumps
         */
        std::string name = readfile.filename().string();
        int j = atoi(name.substr(name.find("_") + 1).c_str());
        std::string first = name.substr(0, name.find("_"));
        if (j > i && fname == first) i= j + 1;

    }
    ///> Get total numbers of frames in video (if possible?)
    int frames = cap.get(CAP_PROP_FRAME_COUNT);
    createTrackbar("seek", "Gandur", &seeker, frames, setSeek);
    int frame = 0;

    while(cap.read(srcImg)) {
        frame = cap.get(CAP_PROP_POS_FRAMES);

        if (seek) {
            seek=false;
            frame = seeker;
            cap.set(CAP_PROP_POS_FRAMES, frame);
        } else {
            ///> move trackbar without seeking video
            ///> to indicate where in video you are
            if (frame % 120 == 0){
                moveSeek =true;
                setTrackbarPos("seek", "Gandur",frame);
            }
        }
        ///> Resize image for better viewing
        double f = srcImg.cols / (double)srcImg.rows;
        resize(srcImg, image, cv::Size(720,(int)(720./f)), 1, 1, CV_INTER_LINEAR);
        double scale = (double)image.cols / srcImg.cols;

        //image=srcImg.clone();
        //net->Detect(srcImg,0.6, 0.5);
        //image = net->drawDetections();

        ///> Draw boxes to indicate what in video gets dumped.
        for (auto box : boxes) {
            rectangle(image, box, CV_RGB(255, 0, 0), 2);
        }
        ///> Show image with boxes
        imshow("Gandur",image);

        ///> Keyboard commands
        int k = waitKey(10);
        if(k==27) break;
        else if (k == 'a') {   ///> Add box to dump in video.
            while (1) {
                Rect test = selectROI("Gandur",image);
                if (test.width!=0 && test.height!=0) {
                    boxes.push_back(test);
                    rectangle(image, test, CV_RGB(0,255,0),2);
                    imshow("Gandur",image);
                } else break;
            }
        } else if (k == 'c') {   ///> Clear all dump boxes
            boxes.clear();
        } else if (k == ' ') {   ///> Space, save each box if frame.

            if (boxes.size()==0) {
                boxes.push_back(Rect(0,0,image.cols,image.rows));
            }

            for (auto box : boxes) {
                std::string newName=fname+"_"+std::to_string(i)+".jpg";
                std::cout <<"trying to save image... nr:" << i << ", " << newName<< "\n";
                Rect sizedBox;
                ///> Save box with original resolution
                sizedBox.x = box.x / scale;
                sizedBox.y = box.y / scale;
                sizedBox.width = box.width  / scale;
                sizedBox.height = box.height / scale;
                ///> Display some information i terminal
                cout << "x:" << box.x << " sc:" << sizedBox.x << endl;
                cout << "w:" << box.width << " sc:" << sizedBox.width << endl;
                ///> Dump image.
                imwrite((p/newName).string(),srcImg(sizedBox),compression_params);
                i++;
            }
        } else if (k == 'f') {       ///> Display current frame position
            std::cout << cap.get(CV_CAP_PROP_POS_FRAMES) << std::endl;
        }

    } //detectionloop
    std::cout << "Cant read image / end of video..\n";

    return 0;
}