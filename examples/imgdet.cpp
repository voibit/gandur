#include "imgdet.hpp"

using namespace Imgdet;

int main(int argc, char**argv){
	workPath = argc>1 ? argv[1] : ".";
	savePath = argc>2 ? argv[2] : workPath/"ok";

	size_t start =  argc>3 ? atoi(argv[3]) : 0;
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


