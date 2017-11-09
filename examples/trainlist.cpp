#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/crc.hpp>
#include <cmath>

using namespace boost::filesystem;
using std::string, std::vector, std::cout, std::endl;

int crc(const string& my_string) {
    boost::crc_ccitt_type result;
    result.process_bytes(my_string.data(), my_string.length());
    return result.checksum();
}

bool isValid(const string &p, float bound) {
	float num = crc(p) / (pow(2,16)-1);
	return num < bound;
}

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

void writeList(const path &p, const vector<string> &list) {

	ofstream file(p);
	for (string p : list) {
		file << p << std::endl; 
	} 
	file.close();
}

int main(int argc, char **argv) {

	vector<string> train;
	vector<string> valid;

	path dirsfile="./data/traindirs.txt";
	path trainlist="../darknet/trainlist.txt";
	path validlist="../darknet/validlist.txt";
	float prob = 0.09;

	if (argc>1) dirsfile=argv[1];
	if (argc>2) trainlist=argv[2];
	if (argc>3) validlist=argv[3];
	if (argc>4) prob=atof(argv[4]);

	//read trainlist
	if (!exists(dirsfile)) {
		cout << "./trainlist dirsfile trainfile validfile validprob\n";
		cout << "dirsfile: "<<dirsfile<<" does not exist.." <<endl;
		return -1;
	}

	ifstream file(dirsfile);
	vector<string> dirs;
	string line;

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

	for (string dir: dirs) {
		for (auto entry : directory_iterator(dir)) {
			if(checkFile(entry.path()) ) {
				string imgpath = canonical(entry).string();
				if (isValid(imgpath, prob)) valid.push_back(imgpath);
				else train.push_back(imgpath);
			}	
		}
	}
	writeList(trainlist, train); 
	writeList(validlist, valid); 

	cout << "Wrote "<< train.size() <<" training images, "<< valid.size() << " validation images" << endl;
	return 0; 
}