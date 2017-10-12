# Makefile to build gandur
#-lopencv_viz 


DARKNET=-I../darknet/include/ -I../darknet/src/ -L/usr/local/lib -ldarknet-cpp-shared -DGPU -DCUDNN 
CUDA=-I/opt/cuda/include/ -L/opt/cuda/lib64 -lcuda -lcudart -lcublas -lcurand
CVBOOST=-I./ `pkg-config --libs opencv` -lboost_system -lboost_filesystem
ALL=$(DARKNET) $(CUDA) $(CVBOOST) -std=c++1z

gandur: clean
	g++ examples/main.cpp gandur.cpp $(ALL) -o gandur
gandur-debug: clean
	g++ examples/main.cpp gandur.cpp -DGPU -D_DEBUG $(ALL) -o gandur
imgdet: clean-img
	g++ examples/imgdet.cpp gandur.cpp $(ALL) -o imgdet
videt: clean-vid
	g++ examples/videt.cpp gandur.cpp $(ALL) -o videt
boxes: clean-boxes
	g++ examples/box.cpp $(CVBOOST) -o boxes
trainlist: clean-trainlist 
	g++ examples/trainlist.cpp $(CVBOOST) -o trainlist
changeClass: clean-changeClass
	g++ examples/changeClass.cpp $(ALL) -o changeClass

clean:
	rm -rf ./gandur
clean-img:
	rm -rf ./imgdet
clean-vid:
	rm -rf ./videt
clean-boxes:
	rm -rf ./boxes
clean-trainlist:
	rm -rf ./trainlist
clean-changeClass:
	rm -rf ./changeClass