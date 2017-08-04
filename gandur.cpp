/*************************************************************************
 * arapaho                                                               *
 *                                                                       *
 * C++ API for Yolo v2                                                   *
 *                                                                       *
 * This test wrapper reads an image or video file and displays           *
 * detected regions in it.                                               *
 *                                                                       *
 * https://github.com/prabindh/darknet                                   *
 *                                                                       *
 * Forked from, https://github.com/pjreddie/darknet                      *
 *                                                                       *
 * Refer below file for build instructions                               *
 *                                                                       *
 * arapaho_readme.txt                                                    *
 *                                                                       *
 *************************************************************************/

#include "arapaho.hpp"
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


#include <opencv2/tracking.hpp>

using namespace cv;

//
// Some configuration inputs
//
static char INPUT_DATA_FILE[]    = "../jk.data"; 
static char INPUT_CFG_FILE[]     = "jk.cfg";
#define MAX_OBJECTS_PER_FRAME (100)

#define TARGET_SHOW_FPS (10)

int xPos(const Rect2d &a) {
    printf("x pos: %f\n", a.x);
    return (a.x+a.width)/2;
}

bool checkX(const std::vector<Rect2d> &a,const std::vector<Rect2d> &b) {
    int margin = 10;
    bool found = false;
    for (int ia = 0; ia < a.size();ia++)
    {
        found = false;
        for (int ib = 0; ib < b.size(); ib++)
        {


            if (xPos(a[ia]) < xPos(b[ib]) + margin && xPos(a[ia]) > xPos(b[ib]) - margin) {
                found = true;
                break;
            } 
        }
        
        if (!found) return false;
    }
    return found;
     /* std::cout << value; ... */
}


void drawBoxes(Mat &image,const std::vector<Rect2d> &obj, const std::vector<std::string> &lab, const std::vector<float> &pro, float aov) {

    for(unsigned int i=0;i<obj.size();i++) {

        printf("drawing boxes.. %f x:%f y:%f w:%f h:%f \n", i, obj[i].x, obj[i].y, obj[i].width, obj[i].height);
        
        //Calculate degrees.
        float dpp = aov / sqrt(image.cols*image.cols + image.rows* image.rows) ;
        float degrees = obj[i].x+obj[i].width/2-image.cols/2;
        degrees *= dpp;
        //DPRINTF("Label:%s prob: %00f \n\n", labels[objId].c_str(),probs[objId]);
        
        //Draw label
        char prob[5];
        sprintf(prob,"%1.2f",pro[i]);

        Point bm0=obj[i].tl(); 
        putText(image, lab[i], bm0, FONT_HERSHEY_DUPLEX, 1, CV_RGB(0, 0, 0), 1, CV_AA);

        bm0.x-=2;
        bm0.y-=1;

        putText(image, lab[i], bm0, FONT_HERSHEY_DUPLEX, 1, CV_RGB(70, 250, 20), 1, CV_AA);   

        bm0.x-=55;

        putText(image, prob, bm0,FONT_HERSHEY_DUPLEX, 0.7,CV_RGB(0, 0, 0),1, CV_AA);
        
        bm0.x-=1;
        bm0.y-=1;

        putText(image, prob, bm0, FONT_HERSHEY_DUPLEX, 0.7,CV_RGB(70, 200, 0),1, CV_AA);
        
        //Draw line from bottom 
        Point bm = Point(obj[i].x+obj[i].width/2, obj[i].y+obj[i].height);
        Point bm2 = Point(obj[i].x, obj[i].y+obj[i].height+14);
        // Draw sircle and line
        circle(image, bm, 4, cvScalar(50, 255, 200), 2);
        line(image, cvPoint(image.cols/2-1, image.rows), bm,cvScalar(0, 0, 0), 2);
        line(image, cvPoint(image.cols/2+1, image.rows), bm, CV_RGB(100, 200, 255), 1);

        // Show image and overlay using OpenCV
        //rectangle(image, obj[i].tl(), obj[i].br(),CV_RGB(100, 200, 255), 1, 8, 0);

        //Draw bearing from center
        char str[9]; 
        sprintf(str,"%3.2f", degrees);
        bm2.x+=50;
        putText(image, str, bm2,
                FONT_HERSHEY_DUPLEX, 1.3, CV_RGB(0, 0, 0), 2, CV_AA);
        bm2.x-=2;
        bm2.y-=1;         
        putText(image, str, bm2,
                FONT_HERSHEY_DUPLEX, 1.3, CV_RGB(70, 250, 20), 1, CV_AA);
    }
}

//
// Some utility functions
// 
bool fileExists(const char *file) 
{
    struct stat st;
    if(!file) return false;
    int result = stat(file, &st);
    return (0 == result);
}

//
// Main test wrapper for arapaho
//
int main(int argn, char** argv)
{
    if (argn < 3) {
        printf("gandur weightsfile avfile [args]\n\n");
        printf("optional arguments\n"); 
        printf(" -stream\tshow or stream result\n");
        printf(" -thresh\tset lower propability limit, defalt: 0.24\n");
        printf(" -notrack\tDo not track using opencv\n");
        return -1;
    }

    //Server pointer;
    HttpServer* server;

    bool stream = find_arg(argn, argv, (const char*)"-stream");
    bool notrack = find_arg(argn, argv, (const char*)"-notrack");
    float aov = find_float_arg(argn, argv, "-aov", 60);
    float thresh =find_float_arg(argn, argv, "-thresh", 0.24);
    int fskip = find_int_arg(argn, argv, "-fskip", 1);

    unsigned int frame = 0;  
    unsigned int lost = 0;
    
    //trackingobject
    std::vector<cv::Rect2d> objects;
    std::vector<std::string> label;
    std::vector<float> prob; 
    bool track = false;
    bool isfound;
    MultiTrackerTLD trackers;


    //Stream or show output image. 
    if (stream) {
        printf("Setting up streamingserver.. \n");
        
        server = new HttpServer();
        server->is_debug = false;
        server->run(8080);
    }
    else {
        namedWindow ( "Gandur" , CV_WINDOW_AUTOSIZE );
    }



    bool ret = false;
    int expectedW = 0, expectedH = 0;
    box* boxes = 0;
    std::string* labels;
    float* probs = 0;
    
    // Early exits
    if(!fileExists(INPUT_DATA_FILE) || !fileExists(INPUT_CFG_FILE) || !fileExists(argv[1])  )
    {
        EPRINTF("Setup failed as input files do not exist or not readable!\n");
        return -1;       
    }
    
    // Create arapaho
    ArapahoV2* p = new ArapahoV2 
    ();
    if(!p)
    {
        return -1;
    }
    
    // TODO - read from arapaho.cfg    
    ArapahoV2Params ap;
    ap.datacfg = INPUT_DATA_FILE;
    ap.cfgfile = INPUT_CFG_FILE;
    ap.weightfile = argv[1];
    ap.nms = 0.4;
    ap.maxClasses = 4;
    
    // Always setup before detect
    ret = p->Setup(ap, expectedW, expectedH); //setup gives network width and height 
    if(false == ret)
    {
        EPRINTF("Setup failed!\n");
        if(p) delete p;
        p = 0;
        return -1;
    }

    // Setup image buffer here
    Mat image;

    // open a video or image file
    VideoCapture cap ( argv[2] );
    if( ! cap.isOpened () )  
    {
        EPRINTF("Could not load the AV file %s\n", argv[2]);
        if(p) delete p;
        p = 0;
        return -1;
    }
    // Detection loop
    while(1)
    {
        bool success = cap.read(image);

        if(!success)
        {
            EPRINTF("cap.read failed/EoF - AV file %s\n", argv[2]);
            if(p) delete p;
            p = 0;
            break;
        }    
        if( image.empty() ) 
        {
            EPRINTF("image.empty error - AV file %s\n", argv[2]);
            if(p) delete p;
            p = 0;
            break;
        }
        else
        {

            // MAIN STUFF! ********************************************************************************
            //*********************************************************************************************

            //Resize image if it is too lagre to view. (will be resized further on to fit network)
            if (image.size().width > 1280) {

                resize(image, image, Size(1280, image.size().height*((double)1280/image.size().width))); 
            } 

            DPRINTF("Image data = %p, w = %d, h = %d\naov= %00f", image.data, image.cols, image.rows, aov);
            
            // Remember the time
            auto detectionStartTime = std::chrono::system_clock::now();
            
            int numObjects = 0;

            // Detect the objects in the image //-thresh and 
            p->Detect(image,thresh,0.5,numObjects);

            std::chrono::duration<double> detectionTime = (std::chrono::system_clock::now() - detectionStartTime);
        
            printf("==> Detected [%d] objects in [%f] seconds\n", numObjects, detectionTime.count());
            
            // Track objects using darknet, and pass to opencv
            if(numObjects > 0 && numObjects < MAX_OBJECTS_PER_FRAME) // Realistic maximum
            {    
                boxes = new box[numObjects];
                probs = new float[numObjects];
                labels = new std::string[numObjects];
                if(!boxes)
                {
                    if(p) delete p;
                    p = 0;
                    return -1;
                }
                if(!labels)
                {
                    if(p) delete p;
                    p = 0;
                    if(boxes)
                    {
                        delete[] boxes;
                        boxes = NULL;                        
                    }
                    return -1;
                }
                
                // Get boxes and labels
                p->GetBoxes(boxes,labels,probs,numObjects);
                
                int objId = 0;
                int leftTopX = 0, leftTopY = 0, rightBotX = 0,rightBotY = 0;

                //reset tracking
                objects.clear();
     
                for (objId = 0; objId < numObjects; objId++)
                {

                    leftTopX = 1 + image.cols*(boxes[objId].x - boxes[objId].w / 2);
                    leftTopY = 1 + image.rows*(boxes[objId].y - boxes[objId].h / 2);
                    rightBotX = 1 + image.cols*(boxes[objId].x + boxes[objId].w / 2);
                    rightBotY = 1 + image.rows*(boxes[objId].y + boxes[objId].h / 2);
                    DPRINTF("Box #%d: center {x,y}, box {w,h} = [%f, %f, %f, %f]\n", 
                            objId, boxes[objId].x, boxes[objId].y, boxes[objId].w, boxes[objId].h);

                    //filling vectors.. 
                    objects.push_back(Rect2d(Point(leftTopX,leftTopY),Point(rightBotX,rightBotY)));
    
                   
                } // loop every objects

                //check x positions..  
                //track=checkX(trackers.getObjects(), objects);
                // || frame%120==0
                if (!track || numObjects > trackers.boundingBoxes.size() || notrack) {

                printf("creating new tracker.. \n");
                   //start tracking
                    //trackers.clear();
                    prob.clear();
                    label.clear();

                    MultiTrackerTLD trackerNew;
                    trackers = trackerNew;

                    label.assign(labels, labels+numObjects);
                    prob.assign(probs, probs+numObjects);

                    printf("Start the tracking process\n");
                    for (auto obj:objects)
                    {
                        trackers.addTarget(image,obj,createTrackerByName("MEDIAN_FLOW"));
                    
                    }
                    track=true;
                }


                if (boxes)
                {
                    delete[] boxes;
                    boxes = NULL;
                }
                if (labels)
                {
                    delete[] labels;
                    labels = NULL;
                }
                if (probs)
                {
                    delete[] probs;
                    probs = NULL;
                }   
                
            }// If objects were detected


            // Track objects using opencv
            drawBoxes(image, trackers.boundingBoxes, label, prob,aov);
            isfound = trackers.update(image);
            if(!isfound)
            {

                printf("The targets has been lost...\n");
                track=false;
                lost++; 
            }



            if (stream) {
                if (frame%fskip == 0) server->IMSHOW("im", image);
            }
            else {
               imshow("Gandur", image); 
            }
            

            char k = waitKey(20);

            if (k=='m') {

                //trackers.clear();
                //need to clear this.

                MultiTrackerTLD trackerNew;
                trackers = trackerNew;

                objects.push_back(selectROI("Gandur",image));
                label.push_back("new");
                prob.push_back(0);


                printf("Start the tracking process\n");
                for (auto obj:objects)
                {
                    trackers.addTarget(image,obj,createTrackerByName("MEDIAN_FLOW"));    /* code */
                }
                    /* code */
                //trackers.addTarget(image,objects,createTrackerByName("MEDIAN_FLOW"));
                lost=0;
                track=true;

            }
            else if (k=='r' || lost > 5) {
                    printf("resetting..\n");

                    //trackers.clear();
                    objects.clear();
                    prob.clear();
                    label.clear();

                    MultiTrackerTLD trackerNew;
                    trackers = trackerNew;
                    track=false;
                    lost=0;
            }
            else if(k==27) break;
            frame++;




         
        } //If a frame was read
    //quit on ESC button

    }// Detection loop
    
clean_exit:    

    // Clear up things before exiting
    if(p) delete p;
    delete server;
    DPRINTF("Exiting...\n");
    return 0;
}       
