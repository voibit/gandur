/**
 *	@file small.cpp
 *	small, https://github.com/voibit/gandur/examples/small.cpp
 *	@brief Program to dump small labeld objects from bigger
 *	@author Jan-Kristian Mathisen
 *	@author Joachim Lowzow
 */
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>

using namespace boost::filesystem;
using namespace cv;
using std::string;
using std::vector;

struct Detection {
    std::string label;
    unsigned int labelId; 
    float prob;
    cv::Rect2f box;
};

vector<path> imgs;
vector<Detection> dets;

/**
 * Save image, convert to darknet format and save label.
 *
 * @param imgpath save path.
 * @param img current image to save
 * @param ids vector with all the ids to resize
 * @param box vector with all boxes to resize
 */
void save(path imgpath, const Mat &img, const vector<int> &ids, vector<Rect> const &box) {
	std::vector<int> compression_params;
	compression_params.push_back(IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	imwrite(imgpath.string(), img, compression_params); ///> Save image
	imgpath.replace_extension(".txt");                  ///> prepare for label save

	/**
	 * Convert from cv to darknet format and save in savepath
	 */
	ofstream file(imgpath);
	for (size_t i = 0; i < box.size(); i++) {
		if (i > 0) file << std::endl;
		file << ids[i];
		file << " " << (box[i].x + box[i].width / 2.) / img.cols;
		file << " " << (box[i].y + box[i].height / 2.) / img.rows;
		file << " " << box[i].width / (float)img.cols;
		file << " " << box[i].height / (float)img.rows;
	}
	file.close();
}

/**
 * Check if file extension is of an image
 * @param p filename to check
 * @return true if image eg *.jpg
 */
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

/**
 * Get vector with paths to all images in folder
 * @param p folder path
 * @return vector with all images in folder
 */
vector<path> getImgs(path p) {
	vector<path> tmp;
	for(auto &entry : directory_iterator(p)) {
		path imgName=entry.path().filename();
		if (isImg(imgName)) {
			tmp.push_back(p / imgName);
		}
	}
	return tmp;  
}

/**
 * Read darkent label file.
 * @param p path to label file
 * @return Vector with Gandur detection class
 */
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

/**
 * Generates a random number in given range
 * @param from start
 * @param to stop
 * @return random number
 */
int random(int from,int to) {
	return from + (rand() % to);
}

/**
 * Convert from darknet format to Opencv.
 * @param img image get dimensions from.
 * @param box darkent box detection parameter
 * @return Opencv Rect object with dimensions
 */
Rect ptoi(const Mat &img, const Rect2f &box) {
	Rect det;

	det.width=box.width*img.cols;
	det.height=box.height*img.rows;

	det.x =  box.x *(float)img.cols - 0.5*det.width;
	det.y =  box.y *(float)img.rows - 0.5*det.height;

	return det;
}

int main(int argc, char**argv){


    const String keys =
    "{help h ?       |        | print this message  }"
    "{@readPath      | .      | Path to images      }"
    "{@savePath      | ./small| path save images in }"
    "{area a         | .001   | max area of small objects}";

    CommandLineParser parser(argc, argv, keys);
    parser.about("Gandur small obj extractor v1.0.0");
    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }
	if (argc < 3) {
		parser.printMessage();
		return -1;
	}
	path p = parser.get<string>("@readPath");
	path savepath=parser.get<string>("@savePath");
	double thresh = parser.get<double>("area");

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

					/**
					 * add random padding, check if it is within image
					 */
					int x = det.x;
					int y = det.y;
					///> Set padding for x
					int xPadLeft =random(5,160);
					int xPadRight =random(5,160);
					if ((x-xPadLeft) < 0) xPadLeft=x;
					det.x = x-xPadLeft;
					///> if random value is outside range, use max;
					det.width = (det.width+ xPadLeft+ xPadRight+ det.x) > img.cols?
								 img.cols-det.x : det.width+xPadLeft+xPadRight;

					///> Set padding for y
					int yPadTop = random(5, 140);
					int yPadBottom = random(5, 140);
					if ((y-yPadTop) < 0) yPadTop=y;
					det.y = y-yPadTop;
					///> if random value is outside range, use max;
					det.height = (det.height+ yPadTop+ yPadBottom+ det.y) > img.rows?
								 img.rows-det.y : det.height+yPadTop+yPadBottom;


					///> change detection coordinates.
					Mat newImg = img(det).clone();
					Rect imgBox(0,0,newImg.cols, newImg.rows);


					///> Move all detections according to current.
					for (size_t j=0; j < dets.size();j++) {
						Rect nbox = ptoi(img, dets[j].box);
						
						nbox.x = nbox.x + xPadLeft - x;
						nbox.y = nbox.y + yPadTop - y;
						///> check if other detections are within new image
						if ((nbox & imgBox).area() > 0) {
							
							nboxes.push_back(nbox);
							classes.push_back(dets[j].labelId);
						}

					}
					///> Save cropped image
					path filename(std::to_string(nr++));
					filename.replace_extension(".jpg");
					save(savepath/filename,newImg,classes,nboxes);

					/* Show boxes
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
