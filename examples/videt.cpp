#include <iostream>
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace boost::filesystem;

int seeker = 0;
bool seek = false;
bool moveSeek = false;
Mat boks;

static void setSeek( int, void* )
{
    if (!moveSeek) seek=true;
    else moveSeek = false;
}

static void on_mouse(int event, int x, int y, int flags, void* ){
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		std::cout << "@ Left mouse button pressed at: " << x << "," << y << std::endl;
	//		icrop(x,y);
	}
	if (event == CV_EVENT_MOUSEMOVE){
    }
}


int main(int argc, char** argv) {

    path p(argc>1? argv[1] : ".");

    path filename = p.filename();
    p = canonical(p).parent_path();
    if (!is_directory(p / "ok")) create_directory(p / "ok");
	
	Mat image = imread("./load.jpg",CV_LOAD_IMAGE_COLOR);
    Mat srcImg;
    std::vector<Rect> boxes;
    std::vector<int> compression_params;
    compression_params.push_back(IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);

	VideoCapture cap ( (p/filename).string() );
    int n=0;

    if( ! cap.isOpened () )  
    {
        std::cout << "Could not open file: "<< p << std::endl; 
        return -1;
    }

    imshow("Gandur",image);
    namedWindow("Gandur",WINDOW_AUTOSIZE);
    moveWindow("Gandur",0,0);
    createTrackbar("seek","Gandur", &seeker, 100, setSeek);

    std::cout << "GANDUR YEAH!\n";
 
    //Gandur *net = new Gandur();
    filename=filename.replace_extension("jpg");
    std::string fname = filename.string(); 
    p/="ok"; 
    //get highest number in folder
    int i=0;

    //destroyWindow("Loading");
    for(auto& entry : directory_iterator(p)) {
    
        std::string name = entry.path().filename().string();
        int j = atoi(name.substr(0, name.find("_")).c_str());
        std::string rest= name.substr(name.find("_")+1);

        if (j > i && fname==rest) i=j;

    }
    int frames = cap.get(CAP_PROP_FRAME_COUNT);
    int frame = 0;

    while(cap.read(srcImg)) {
        frame =cap.get(CAP_PROP_POS_FRAMES);

        if (seek) {
            frame = seeker/100.*frames;
            cap.set(CAP_PROP_POS_FRAMES,frame);
            seek=false;
        }
        else {
            //move trackbar without seeking video 
            if (frame %120==0){
                moveSeek =true;
                setTrackbarPos("seek", "Gandur",(int)(100./frames*frame)); 
            } 
        }

        image=srcImg.clone();
        
    	//net->Detect(srcImg,0.6, 0.5);
        //image = net->drawDetections();
        
        for (auto box : boxes) {
            rectangle(image, box, CV_RGB(255,0,0),2);  
        } 

    	imshow("Gandur",image);
        int k = waitKey(10);

		if(k==27) break;
        else if(k=='a') {
            while (1) {
                Rect test = selectROI("Gandur",image);
                if (test.width!=0 && test.height!=0) {
                    boxes.push_back(test);
                    rectangle(image, test, CV_RGB(0,255,0),2);
                    imshow("Gandur",image);  
                }
                else break;
            }
	    }
        else if(k=='c') {
            boxes.clear();
        }
        else if(k==32) {

            if (boxes.size()==0) {
                boxes.push_back(Rect(0,0,image.cols,image.rows));
            }
					
            for (auto box : boxes) {
                std::string newName=std::to_string(i)+"_"+fname;
                std::cout <<"trying to save image... nr:" << i << ", " << newName<< "\n";
                imwrite((p/newName).string(),srcImg(box),compression_params);
                i++;
            }
        }
        else if(k=='f') {
            std::cout << cap.get(CV_CAP_PROP_POS_FRAMES) << std::endl;
        }
        else if(k=='s') {
            //cvSetCaptureProperty(cap,)
        }

    } //detectionloop
    std::cout << "Cant read image / end of video..\n";

    //delete net;
    //net = 0;

	return 0;
}
