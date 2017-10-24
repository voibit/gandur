#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <algorithm>
#include <boost/filesystem.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;

int netx = 608;
int nety = 608;
Mat currentImage;
Mat nextImage;
Mat croppedImage;
Mat copyImage;
string m_winname;
Point p;
int cropNr = 0;

static void on_mouse(int, int, int, int, void*);
void icrop(int, int);
bool loadNext();
void Prev();
void Next();
void loadimg(size_t);
void saveCrop(path f, int i, path dir, Mat image);
void makeRect(int x,int y, Mat &image);

path workPath;
path savePath;
path nextImgName;
path imgName;
vector<path> imgs;
vector<path> getImgs(path p);

size_t cnt = 0;
size_t current = 0;

static void on_mouse(int event, int x, int y, int flags, void* ){
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		std::cout << "@ Left mouse button pressed at: " << x << "," << y << std::endl;
	
		icrop(x,y);
    }
	if (event == CV_EVENT_MOUSEMOVE){
		Mat overlay = copyImage.clone(); 			
		makeRect(x, y, overlay);
		imshow("crop", overlay);
		
	}
}

void Next(){
	cropNr = 0;
	loadNext();
	copyImage = currentImage.clone();
	imshow("crop", currentImage);
}

bool loadNext(){
	bool ret = (current + 2) < cnt;
	std::cout << current << " " << ret << std::endl;
	currentImage = nextImage;
	if(ret){ 
		loadimg(current + 2);
	}
	current++;
	return ret;
	}

void loadimg(size_t i){
	nextImgName=imgs[i].filename();
	nextImage = imread(imgs[i].string());
}

void show(){
	imshow("crop", currentImage);
	}

void icrop(int x, int y){
	int xx, yy;
	xx = netx;
	yy = nety;

	int X = x;
	int Y = y;

	if(currentImage.cols < netx) xx = currentImage.cols;
	if(currentImage.rows < nety) yy = currentImage.rows;
	if(x < (xx/2)) x = (xx/2);
	if(y < (yy/2)) y = (yy/2);

	Rect r = Rect(	min(x - (xx/2), currentImage.cols - (xx)),
					min(y - (yy/2), currentImage.rows - (yy)),  xx, yy);
	std::cout << r.x << " " << r.y << " " << r.width << " " << r.height << endl;
	Mat crop = currentImage(r);
	
	saveCrop(imgs[current], cropNr, savePath, crop);
	cropNr++;

	rectangle(copyImage, r, 1, 8, 0);
	imshow("crop", copyImage);
}

void makeRect(int x,int y, Mat &image){
	if(currentImage.cols < netx) netx = currentImage.cols;
	if(currentImage.rows < nety) nety = currentImage.rows;
	if(x < (netx/2)) x = (netx/2);
	if(y < (nety/2)) y = (nety/2);

	Rect r = Rect(	min(x - (netx/2), currentImage.cols - (netx)),
					min(y - (nety/2), currentImage.rows - (nety)),  netx, nety);
	rectangle(image, r, 1, 8, 0);
}

void saveCrop(path f, int i, path dir, Mat image){
	std::cout << dir.string()+"/"+f.stem().string()+"_"+to_string(i)+".bmp" << std::endl;
	imwrite(dir.string()+"/"+f.stem().string()+"_"+to_string(i)+".bmp", image);
}

bool isImg(path p) {
	string ext=extension(p);
	bool ret = ext==".jpg";
	ret |= ext==".JPG";
	ret |= ext==".jpe";
	ret |= ext==".JPE";
	ret |= ext==".jpeg";
	ret |= ext==".JPEG";	
	ret |= ext==".png";
	ret |= ext==".PNG";
	ret |= ext==".bmp";
	return ret;
}

vector<path> getImgs(path p) {
	vector<path> tmp;
	for(auto &entry : directory_iterator(p)) {
		imgName=entry.path().filename();
		if (isImg(imgName)) {
			tmp.push_back(p/imgName);
		}
	}
	sort(tmp.begin(), tmp.end());
	return tmp;
}
 
int main (int argc, char **argv)
{
	workPath = argc>1 ? argv[1] : ".";
	savePath = argc>2 ? argv[2] : workPath/"crops";

	if (!is_directory(workPath)) {
		std::cout <<"Error: "<<workPath<< " is not a directory. "<< std::endl;
		return -1;  
	}
	if (!is_directory(savePath)) create_directory(savePath);
	workPath=canonical(workPath);
	savePath=canonical(savePath);	

	imgs = getImgs(workPath);
	cnt = imgs.size();
	
	if (cnt == 0) {
		std::cout <<"Error: "<<workPath<<" contains no images. "<< std::endl;
}
	std::cout << "Loaded: " << cnt << " images" << std::endl;

	currentImage = imread(imgs[current].string());
	nextImage = imread(imgs[current + 1].string());

	copyImage = currentImage.clone();
	imshow("crop", currentImage);

	setMouseCallback("crop", on_mouse, NULL);
	while(true){

		int k = waitKey(0);
	    switch(k){
			case 83:
			std::cout << "NEXT!" << std::endl;
			Next();
			case 110:
			std::cout << "NEXT!" << std::endl;
			Next();
	}
}
    return 0;
}
