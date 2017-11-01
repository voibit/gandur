#include "gandur.hpp"

int main(int argn, char** argv) {
	
	cuda_set_device(1);
	Gandur *net = new Gandur();

	net->validate();
	return 0;
}