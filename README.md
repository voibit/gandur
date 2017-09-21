
# Gandur

C++ wrapper class for darknet.
based upon prabindhs "arapaho" [https://github.com/prabindh/darknet].

**Build instructions**

Tested with
* opencv 3.3.0 (compiled with contrib and cuda)
* boost 1.65
* cuda 9 rc
* cudnn 7  

Make shure you have the right paths to cuda set in the makefile, and that you have made a shared lib out of darknet before building.

```bash
make videt
make imgdet
make gandur
```

**Useage**

```c++
#include "gandur.hpp"

Gandur *net= new Gandur();  
net->Detect(Mat,thersh);  
Mat detimg = drawDetections();  
vector<Rect> detRects = net->detections;  
delete net;
```  


## imgdet

Uses darknet to generate more training data from folder with images. 

**Useage**

./imdet folder

Key | effect 
--- | --- | 
enter or space |	correct detection.
c | manual classification
tab | incomplete detection.
d | delete image.
other | keep for manual boxing. 


## videt

A tool to grab different cropped image frames from video.  

**Useage**

./videt videofile.mp4

Key | boxmode | grabmode
--- | --- | --- | 
a | add boxes. | 
c | finish adding boxes | clear boxes
space | select box | save image