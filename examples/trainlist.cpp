#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/crc.hpp>
#include <cmath>
#include <sstream>

using namespace boost::filesystem;
using std::string;
using std::vector;
using std::cout;
using std::endl;

/**
 * Get crc form filepath
 * Used in randomomization
 * @param my_string path to get checksum from
 * @return checksum
 */
int crc(const string& my_string) {
	boost::crc_ccitt_type result;
	result.process_bytes(my_string.data(), my_string.length());
	return result.checksum();
}

/**
 * Check if path should be va valid path
 * @param p string to check
 * @param upper limit for valid  eg .1 for 10%
 * @return true if valid set
 */
bool isValid(const string &p, float bound) {
	return (crc(p) / (pow(2, 16) - 1)) < bound;
}

/**
 * Check if filename contains image extension
 * @param file filename
 * @return true if image
 */
bool checkFile(path file) {
	if (!(extension(file) == ".JPG" ||
		  extension(file) == ".jpg" ||
		  extension(file) == ".jpeg" ||
		  extension(file) == ".bmp" ||
		  extension(file) == ".BMP" ||
		  extension(file) == ".jpe"
	)) return false;
	return exists(file.replace_extension(".txt"));
}

/**
 * Write vector with paths to list file
 */
void writeList(const path &p, const vector<string> &list) {

	ofstream file(p);
	for (string p : list) {
		file << p << std::endl; 
	} 
	file.close();
}
/**
 * Count class ids
 * @param [in] path to image or labelfile
 * @param [out] vector with class count
 */
void countP(path p, vector<int> &countC) {
	p.replace_extension(".txt");
	ifstream file(p);

	string line;
	while (getline(file, line)) {
		int id;
		std::stringstream ss(line);
		ss >> id;
		///> Make shure within range and +1
		if (id < countC.size()) countC[id]++;
	}
	file.close();
}

int main(int argc, char **argv) {

	vector<string> train;
	vector<string> valid;

	path dirsfile="./data/traindirs.txt";
	path trainlist="../darknet/trainlist.txt";
	path validlist="../darknet/validlist.txt";
	float prob = 0.09; 			///> How much of the set to be valid
	vector<int> countC(10, 0); 	///> Count for the different classes

	///> Parse terminal arguments
	if (argc>1) dirsfile=argv[1];
	if (argc>2) trainlist=argv[2];
	if (argc>3) validlist=argv[3];
    if (argc > 4) prob = (float) atof(argv[4]);

	//read trainlist
	if (!exists(dirsfile)) {
		cout << "./trainlist dirsfile trainfile validfile validprob\n";
		cout << "dirsfile: "<<dirsfile<<" does not exist.." <<endl;
		return -1;
	}

	///> Read all directories listed in dirsfile
	ifstream file(dirsfile);
	vector<string> dirs;
	string line;

	///> Add directories to vector
	while (getline(file,line)) {
		if (is_directory(line)) dirs.push_back(line);
		else cout <<"Dir "<<line<< " does not exist.."<<endl;
	}
	file.close(); 

	if (dirs.size()==0) {
		cout <<"no dirs in dirfile.. "<<endl;
		return -1;
	}
	cout << "looping through it.." << endl;

	/**
	 * Loop through each image in each directory.
	 * Check of it should be a valid image or train image.
	 * Add to list vector
	 */
	for (string dir: dirs) {
        for (auto &entry : directory_iterator(dir)) {
			if(checkFile(entry.path()) ) {
				string imgpath = canonical(entry).string();

				countP(imgpath, countC); ///> Count different classes.
				///> Add to valid or trainlist
				if (isValid(imgpath, prob)) valid.push_back(imgpath);
				else train.push_back(imgpath);
			}	
		}
	}
	///> Write vector list to train and valid file.
	writeList(trainlist, train); 
	writeList(validlist, valid); 

	///> Display some nice info. 
	cout << "Wrote "<< train.size() <<" training images, ";
	cout << valid.size() << " validation images" << endl;
	cout << "class stats:\n";
	for (int i =0; i < countC.size(); i++) {
		cout << "\t " << i << " " << countC[i] << endl;
	}
	return 0; 
}