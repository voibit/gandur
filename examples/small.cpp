
#include <boost/filesystem.hpp>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

using namespace boost::filesystem;
using namespace cv; 
using std::string, std::vector;

struct Detection {
    std::string label;
    unsigned int labelId; 
    float prob;
    cv::Rect2f box;
};

vector<path> imgs;
vector<Detection> dets;

void save(path imgpath, const Mat &img, const vector<int> &ids, vector<Rect> const &box) {
	std::vector<int> compression_params;
    compression_params.push_back(IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);

	imwrite(imgpath.string(),img,compression_params);
	imgpath.replace_extension(".txt");

	ofstream file(imgpath);
	for (size_t i = 0; i<box.size();i++) {
		if (i > 0) file << std::endl;
		file << ids[i];
		file << " " << (box[i].x + box[i].width/2.)/img.cols;
		file << " " << (box[i].y + box[i].height/2.)/img.rows;
		file << " " << box[i].width / (float)img.cols;
		file << " " << box[i].height / (float)img.rows;  
	}
	file.close();
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
	return ret;
}

vector<path> getImgs(path p) {
	vector<path> tmp; 
	for(auto &entry : directory_iterator(p)) {
		path imgName=entry.path().filename();
		if (isImg(imgName)) {
			tmp.push_back(p/imgName);
		}
	}
	return tmp;  
}


vector<Detection> readTxt(path p) {
	vector <Detection> dets;
	string line;
	ifstream file(p);
	float x, y, w, h;
	int id; 

	while (getline(file, line)) {
		Detection det; 
		std::istringstream ss(line);
		ss >> id >> x >> y >> w >> h;

		det.box=Rect2f(x,y,w,h); 
		det.labelId=id;
		det.prob=1;
		dets.push_back(det); 
	}
	return dets;
}

int random(int from,int to) {
	return from + (rand() % to);
}

Rect ptoi(const Mat &img, const Rect2f &box) {
	Rect det;

	det.width=box.width*img.cols;
	det.height=box.height*img.rows;

	det.x =  box.x *(float)img.cols - 0.5*det.width;
	det.y =  box.y *(float)img.rows - 0.5*det.height;

	return det;
}

int main(int argc, char**argv){
	path p(argc>1? argv[1] : ".");

	if (argc < 3) {
		std::cout << "small [readpath] [savepath] [thresh]\n";
		return -1;
	}
	path savepath=argv[2];

	double thresh = atof(argv[3]); 

	srand(time(NULL));

	size_t nr=0;

	for (auto txt : getImgs(p)) {

		Mat img = imread(txt.string());

	

		if (exists(txt.replace_extension(".txt"))) {
			dets = readTxt(txt);

			for (size_t i=0; i < dets.size();i++) {
				if (dets[i].box.width*dets[i].box.height < thresh) {

					
					Rect det = ptoi(img, dets[i].box);

					//Stores values for new labels
					vector<Rect> nboxes;
					vector<int> classes;

					//add padding, check if it is within image

					int x = det.x;
					int y = det.y;
					//X
					int xPadLeft =random(5,160);
					int xPadRight =random(5,160);
					if ((x-xPadLeft) < 0) xPadLeft=x;
					det.x = x-xPadLeft;
					det.width = (det.width+ xPadLeft+ xPadRight+ det.x) > img.cols?
								 img.cols-det.x : det.width+xPadLeft+xPadRight;

					//y
					int yPadTop =random(5,150);
					int yPadBottom =random(5,150);
					if ((y-yPadTop) < 0) yPadTop=y;
					det.y = y-yPadTop;
					det.height = (det.height+ yPadTop+ yPadBottom+ det.y) > img.rows?
								 img.rows-det.y : det.height+yPadTop+yPadBottom;


					//change detection coordinates. 
					Mat newImg = img(det).clone();
					Rect imgBox(0,0,newImg.cols, newImg.rows);


					//Move all detections according to current. 
					for (size_t j=0; j < dets.size();j++) {
						Rect nbox = ptoi(img, dets[j].box);
						
						nbox.x = nbox.x + xPadLeft - x;
						nbox.y = nbox.y + yPadTop - y;
						//check if other detections are within new image
						if ((nbox & imgBox).area() > 0) {
							
							nboxes.push_back(nbox);
							classes.push_back(dets[j].labelId);

						}

					}
					path filename(std::to_string(nr++));
					filename.replace_extension(".jpg");

					save(savepath/filename,newImg,classes,nboxes);

					/*
					for (auto nbox : nboxes) {
						rectangle(newImg, nbox,CV_RGB(100, 200, 255), 1, 8, 0);
					}
					imshow("Saving..",newImg);
					waitKey(0);
					*/
					
				}

			}


		}
	}
	return 0;
}
