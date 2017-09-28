#include <iostream>
#include <string>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;
using std::string, std::vector, std::cout, std::endl;

path trainlist;
vector<string> list;

void checkFile(path file) {
	path imgpath = file; 
	if (!(extension(file) == ".JPG" ||
		extension(file) == ".jpg" ||
		extension(file) == ".jpeg" ||
		extension(file) == ".jpe"
		)) return;
	if (!exists(file.replace_extension(".txt"))) return;
	list.push_back(canonical(imgpath).string());
	return;
}

void writeList() {
	cout << "writing trainfile: "<<trainlist <<endl; 
	ofstream file(trainlist);

	for (string p : list) {
		file << p << std::endl; 
	} 
	file.close();
}

int main(int argc, char **argv) {

	path dirsfile="./data/traindirs.txt";
	trainlist="../darknet/trainlist.txt";
	if (argc>1) dirsfile=argv[2];
	if (argc==3) trainlist=argv[3];

	//read trainlist
	if (!exists(dirsfile)) {
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

	if (dirs.size()>0) cout << "looping through it.." << endl;
	else {
		cout <<"no dirs in file.."<<endl;
		return -1;
	}
	for (string dir: dirs) {
		for (auto entry : directory_iterator(dir)) {
			checkFile(entry.path());
		}
	}
	writeList(); 
	return 0; 
}