# Makefile to build gandur
#-lopencv_viz 

DARKNET=-I../darknet/include/ -I../darknet/src/ -L/usr/local/lib -ldarknet -DGPU -DCUDNN 
CUDA=-I/opt/cuda/include/ -L/opt/cuda/lib64 -lcuda -lcudart -lcublas -lcurand
CVBOOST=-I./ `pkg-config --libs opencv` -lboost_system -lboost_filesystem
ALL=$(DARKNET) $(CUDA) $(CVBOOST) -std=c++1z

gandur: clean
	g++ examples/main.cpp gandur.cpp $(ALL) -o gandur
gandur-debug: clean
	g++ examples/main.cpp gandur.cpp -DGPU -D_DEBUG $(ALL) -o gandur
imgdet: clean-img
	g++ examples/imgdet.cpp C $(ALL) -o imgdet
videt: clean-vid
	g++ examples/videt.cpp $(CVBOOST) -o videt
trainlist: clean-trainlist 
	g++ examples/trainlist.cpp $(CVBOOST) -o trainlist
crop: clean-crop
	g++ examples/crop.cpp $(CVBOOST) -o crop
changeClass: clean-changeClass
	g++ examples/changeClass.cpp $(ALL) -o changeClass
small: clean-small
	g++ examples/small.cpp $(CVBOOST) -o small
valid: clean-valid
	g++ examples/validate.cpp gandur.cpp $(ALL) -o valid
	g++ examples/validate1.cpp gandur.cpp $(ALL) -o valid1


clean:
	rm -rf ./gandur
clean-img:
	rm -rf ./imgdet
clean-vid:
	rm -rf ./videt
clean-trainlist:
	rm -rf ./trainlist
clean-crop:
	rm -rf ./crop
clean-changeClass:
	rm -rf ./changeClass
clean-small:
	rm -rf ./small
clean-valid:
	rm -rf ./valid
	rm -rf ./valid1