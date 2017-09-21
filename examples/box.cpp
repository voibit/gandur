//iterates trough a folder with images and darknet .txt file
//displays boxes 

#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#include <string>

using namespace boost::filesystem;
using namespace cv;


bool chkFile(path p) {
	bool ret = (p.extension()==".jpg") || (p.extension()==".jpeg");
	ret *= exists(p.replace_extension(".txt"));
	return ret;   
}

int main(int argc, char**argv){
	path p(argc>1? argv[1] : ".");

	Mat img;

	for(auto& entry : directory_iterator(p)) {
        path imgfile = canonical(entry.path());
        if (chkFile(imgfile)) {
        	img = imread(imgfile.string());

        	float x, y, w, h;
        	int id, X, Y, W, H;  
        	fstream file(imgfile.replace_extension(".txt"),  std::ios::in);
        	std::string line; 
        	
        	while(getline(file, line)) {
        		std::istringstream ss(line);
        		ss >> id >> x >> y >> w >> h;
        		
        		W = w*img.cols; 
        		H = h*img.rows; 
        		X = x*img.cols-0.5*W;
        		Y = y*img.rows-0.5*H;
        		Rect box(X,Y,W,H); 

        		putText(img, std::to_string(id), Point(box.x,box.y), FONT_HERSHEY_DUPLEX, 1, CV_RGB(0, 0, 0), 1, CV_AA);
                rectangle(img, box,CV_RGB(100, 200, 255), 1, 8, 0);
        	}

        	imshow("Gandur",img); 

        	int k = waitKey(0);

        } 
	}
	return 0;
}