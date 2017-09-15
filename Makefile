# Makefile to build the test-wrapper for Arapaho
# Undefine GPU, CUDNN if darknet was built without these defined. These 2 flags have to match darknet flags.
# https://github.com/prabindh/darknet
#-lopencv_viz 


DARKNET=-I../darknet/include/ -I../darknet/src/ -L/usr/local/lib -ldarknet-cpp-shared
CUDA=-I/opt/cuda/include/ -L/opt/cuda/lib64 -lcuda -lcudart -lcublas -lcurand
COMMON=$(DARKNET) $(CUDA) -I./ `pkg-config --libs opencv` -lboost_system -lboost_filesystem


gandur: clean
	g++ examples/main.cpp gandur.cpp -DGPU -DCUDNN $(COMMON) -o gandur
gandur-debug: clean
	g++ examples/main.cpp gandur.cpp -DGPU -D_DEBUG -DCUDNN $(COMMON) -o gandur
imgdet: clean-img
	g++ examples/imgdet.cpp gandur.cpp -DGPU -DCUDNN $(COMMON) -o imgdet
videt: clean-vid
	g++ examples/videt.cpp gandur.cpp -DGPU -DCUDNN $(COMMON) -o videt

clean:
	rm -rf ./gandur
clean-img:
	rm -rf ./imgdet
clean-vid:
	rm -rf ./videt

