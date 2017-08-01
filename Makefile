# Makefile to build the test-wrapper for Arapaho
# Undefine GPU, CUDNN if darknet was built without these defined. These 2 flags have to match darknet flags.
# https://github.com/prabindh/darknet
#-lopencv_viz 
gandur: clean
	g++ gandur.cpp arapaho.cpp Http_server.cpp -DGPU -DCUDNN -I../src/ -I/opt/cuda/include/ -L./ -ldarknet-cpp-shared -L/usr/local/lib `pkg-config --libs opencv` -L/opt/cuda/lib64 -lcuda -lcudart -lcublas -lcurand -lboost_system -lboost_thread -o gandur

gandur-nogpu: clean
	g++ gandur.cpp arapaho.cpp Http_server.cpp -D_DEBUG -I../src/ -L./ -L/usr/local/lib -ldarknet-cpp-shared `pkg-config --libs opencv` -lboost_system  -lboost_thread -std=c++14 -o gandur


clean:

	rm -rf ./gandur	
