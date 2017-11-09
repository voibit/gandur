#include "gandur.hpp"

using namespace cv;
using namespace boost::filesystem;
using std::vector;
using std::string;
using std::to_string;

Mat origImg;
Mat img;
string message;
size_t count = 0;
size_t current = 0;

vector<path> getImgs(path p);

bool doresize = true;


Gandur *net = 0;
path imgName;
path txtName;
path workPath;
path savePath;
vector<path> imgs;
vector<Detection> dets;
vector<string> classes;

bool isImg(path p);

bool delImg();

bool delImg(size_t i);

void saveTxt(path p);

void readTxt(path p);

void next();

void prev();

void show(size_t i);

void draw();

void banner();

bool label();

void save();

void loopImgs(size_t start = 0);


int main(int argc, char**argv){
	workPath = argc>1 ? argv[1] : ".";
	savePath = argc>2 ? argv[2] : workPath/"ok";

	size_t start =  argc>3 ? atoi(argv[3])+1 : 0;
	std::cout << start << std::endl; 

	if (!is_directory(workPath)) {
		std::cout <<"Error: "<<workPath<< " is not a directory. "<< std::endl;
		return -1;  
	}
	if (!is_directory(savePath)) create_directory(savePath);
	workPath=canonical(workPath);
	savePath=canonical(savePath);

	net = new Gandur(); 
	imgs = getImgs(workPath); 
	count = imgs.size();
	if (count == 0) {
		std::cout <<"Error: "<<workPath<<" contains no images. "<< std::endl;
		return -1;
	}
	classes = net->getClasses();

	loopImgs(start);
	return 0; 
}


void loopImgs(size_t start) {
	namedWindow("Gandur", WINDOW_AUTOSIZE);
	moveWindow("Gandur", 0, 0);
	show(start);

	while (label()) {}

}

vector<path> getImgs(path p) {
	vector<path> tmp;
	for (auto &entry : directory_iterator(p)) {
		imgName = entry.path().filename();
		if (isImg(imgName)) {
			tmp.push_back(p / imgName);
		}
	}

	sort(tmp.begin(), tmp.end());
	return tmp;
}

void next() {
	if ((current + 1) < count) current++;
	show(current);
}

void prev() {
	if (current > 0) current--;
	show(current);
}

void show(size_t i) {
	current = i;
	imgName = imgs[i].filename();
	txtName = imgName;
	txtName.replace_extension(".txt");

	origImg = imread(imgs[i].string());


	if (doresize) {
		double factor = 1088. / origImg.rows;
		resize(origImg, origImg, cv::Size(0, 0), factor, factor, CV_INTER_LINEAR);
	}

	img = origImg.clone();
	net->Detect(img);

	if (exists(savePath / imgName) && exists(savePath / txtName)) {

		readTxt(savePath / txtName);
		if (net->detections.size() > dets.size()) {
			message = "labeled, however more results from network.";
		} else {
			message = "already labeled.";
		}

	} else {
		dets = net->detections;
		if (dets.size() > 0) message = "Network results";
		else message = "No detections";
	}
	draw();
}

bool delImg() {
	return delImg(current);
}

bool delImg(size_t i) {
	if (i > count) return false;
	else {
		remove(imgs[i]);
		imgs.erase(imgs.begin() + i);
		count--;
		if (current > count) current--;
		return true;
	}
}

bool isImg(path p) {
	string ext = extension(p);
	bool ret = ext == ".jpg";
	ret |= ext == ".JPG";
	ret |= ext == ".jpe";
	ret |= ext == ".JPE";
	ret |= ext == ".jpeg";
	ret |= ext == ".JPEG";
	ret |= ext == ".png";
	ret |= ext == ".PNG";

	ret |= ext == ".bmp";
	ret |= ext == ".BMP";
	return ret;
}

void readTxt(path p) {
	dets.clear();
	string line;
	ifstream file(p);
	float x, y, w, h;
	int id, X, Y, W, H;

	while (getline(file, line)) {
		Detection det;
		std::istringstream ss(line);
		ss >> id >> x >> y >> w >> h;

		W = w * img.cols;
		H = h * img.rows;
		X = x * img.cols - W / 2;
		Y = y * img.rows - H / 2;
		det.box = Rect(X, Y, W, H);
		det.labelId = id;
		if (net) det.label = net->getLabel(id);
		det.prob = 1;
		dets.push_back(det);
	}
}

void banner() {
	Rect box(0, 0, img.cols, 26);
	Scalar rgba(0, 0, 0, 0.7);
	double alpha = rgba[3] == 0 ? 1 : rgba[3];
	Scalar rgb(rgba[0], rgba[1], rgba[2]);
	Mat roi = img(box);
	Mat color(roi.size(), CV_8UC3, rgb);
	addWeighted(color, alpha, roi, 1.0 - alpha, 0.0, roi);
}

void draw() {
	//start with a clean image
	img = origImg.clone();

	//draw banner;
	banner();

	//draw picnr
	string picnr = "|" + to_string(current + 1) + ":" + to_string(count);
	picnr += "| " + imgName.string();
	putText(img, picnr, Point(0, 20), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(30, 230, 150), 1, CV_AA);

	//Draw message
	Size textSize = getTextSize(message, FONT_HERSHEY_DUPLEX, 0.5, 1, 0);
	putText(img, message, Point(img.cols - textSize.width, 20), FONT_HERSHEY_DUPLEX, 0.5, CV_RGB(30, 230, 150), 1,
			CV_AA);

	//Draw detections
	for (auto det : dets) {
		rectangle(img, det.box, CV_RGB(100, 200, 255), 1, 8, 0);
		Point txtpos(det.box.x + 2, det.box.y + 1);
		putText(img, det.label, txtpos, FONT_HERSHEY_DUPLEX, 0.6, CV_RGB(0, 0, 0), 1, CV_AA);
		txtpos.x -= 2;
		txtpos.y -= 1;
		putText(img, det.label, txtpos, FONT_HERSHEY_DUPLEX, 0.6, CV_RGB(0, 255, 50), 1, CV_AA);
		textSize = getTextSize(det.label, FONT_HERSHEY_DUPLEX, 0.6, 1, 0);
		txtpos.x += textSize.width - 2;
		int prob = det.prob * 100.;
		putText(img, to_string(prob) + "%", txtpos, FONT_HERSHEY_DUPLEX, 0.4, CV_RGB(0, 0, 0), 1, CV_AA);
		txtpos.x -= 2;
		txtpos.y -= 1;
		putText(img, to_string(prob) + "%", txtpos, FONT_HERSHEY_DUPLEX, 0.4, CV_RGB(0, 210, 20), 1, CV_AA);
	}
	imshow("Gandur", img);
}


bool label() {

	message = "press 0-9 for class id, r reset, s save, c cancel";
	draw();
	int k = waitKey(0);
	if (k == 'q' || k == 27) return false;
	if (k == '|') k = 48;
	if (k > 47 && k < 48 + classes.size()) {
		int id = k - 48;

		message = classes[id];
		draw();

		Rect box = selectROI("Gandur", img);

		if (box.width != 0 && box.height != 0) {
			Detection det;
			det.box = box;
			det.labelId = id;
			det.label = classes[id];
			dets.push_back(det);
			draw();
		}
	}

	switch (k) {
		case 81: //left
			prev();
			break;
		case 83: //right
			next();
			break;
			//save
		case 10: //enter
		case ' '://space
		case 's':
			save();
			next();
			break;
			//reset boxing.
		case 'r':
			dets.clear();
			draw();
			break;
			//get from network
		case 'n':
			dets.clear();
			dets = net->detections;
			message = "network results";
			draw();
		case 'd':
			message = "Are you shure you want to delete this file?(y/N):";
			draw();
			if (waitKey(0) == 'y') {
				delImg();
				next();
			}
			break;
		default:
			return true;
			break;
	}
	return true;
}

void save() {
	copy_file(workPath / imgName, savePath / imgName, copy_option::overwrite_if_exists);
	saveTxt(savePath / txtName);
	message = "labels saved.. ";
	draw();
}

void saveTxt(path p) {
	ofstream file(savePath / txtName);
	for (size_t i = 0; i < dets.size(); i++) {
		if (i > 0) file << std::endl;
		file << dets[i].labelId;
		file << " " << (dets[i].box.x + dets[i].box.width / 2.) / img.cols;
		file << " " << (dets[i].box.y + dets[i].box.height / 2.) / img.rows;
		file << " " << dets[i].box.width / (float) img.cols;
		file << " " << dets[i].box.height / (float) img.rows;
	}
	file.close();
}
