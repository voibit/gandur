
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

void saveTxt(path p) {
	ofstream file(p);
	for (size_t i = 0; i<dets.size();i++) {
		if (i > 0) file << std::endl;
		file << dets[i].labelId;
		file << " " << dets[i].box.x;
		file << " " << dets[i].box.y;
		file << " " << dets[i].box.width;
		file << " " << dets[i].box.height;  

	}
	file.close();
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

int main(int argc, char**argv){
	path p(argc>1? argv[1] : ".");

	if (argc < 3) {
		std::cout << "changeClass [path] from to\n";
		return -1;
	}
	int from = atoi(argv[2]); 
	int to = atoi(argv[3]); 

	bool save=false;

	for (auto txt : getImgs(p)) {

		if (exists(txt.replace_extension(".txt"))) {
			dets = readTxt(txt);
			for (size_t i=0; i < dets.size();i++) {
				if (dets[i].labelId==from) {
					dets[i].labelId=to;
					save=true;
				}
			}
			if (save) {
				std::cout << "changing " << txt << std::endl;
				save=false;
				saveTxt(txt);

			}
		}
	}
	return 0;
}