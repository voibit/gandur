# Makefile to build the test-wrapper for Arapaho
# Undefine GPU, CUDNN if darknet was built without these defined. These 2 flags have to match darknet flags.
# https://github.com/prabindh/darknet
#-lopencv_viz 
gandur: clean
	g++ main.cpp gandur.cpp -DGPU -DCUDNN -I../darknet/include/ -I../darknet/src/ -I/opt/cuda/include/ -L./ -ldarknet-cpp-shared -L/usr/local/lib `pkg-config --libs opencv` -L/opt/cuda/lib64 -lcuda -lcudart -lcublas -lcurand -lboost_system -lboost_thread -o gandur
gandur-debug: clean
	g++ main.cpp gandur.cpp -DGPU -D_DEBUG -DCUDNN -I../darknet/include -I../darknet/src/ -I/opt/cuda/include/ -L./ -ldarknet-cpp-shared -L/usr/local/lib `pkg-config --libs opencv` -L/opt/cuda/lib64 -lcuda -lcudart -lcublas -lcurand -lboost_system -lboost_thread -o gandur


gandur-nogpu: clean
	g++ gandur.cpp arapaho.cpp Http_server.cpp -w -D_DEBUG -I../darknet/src/ -L./ -L/usr/local/lib -ldarknet-cpp-shared `pkg-config --libs opencv` -lboost_system  -lboost_thread -std=c++14 -o gandur


clean:

	rm -rf ./gandur

