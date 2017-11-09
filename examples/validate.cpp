#include "gandur.hpp"

int main(int argc, char **argv) {
	path p(argc > 1 ? argv[1] : "");
	path validlist(argc > 2 ? argv[2] : "");
	
	Gandur *net = new Gandur();

	net->validate(p, validlist);
	return 0;
}