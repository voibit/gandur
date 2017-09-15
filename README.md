
# Gandur

C++ wrapper class for darknet.
based upon prabindhs "arapaho". [https://github.com/prabindh/darknet]

**Useage**

Gandur *net= new Gandur(); _ 
net->Detect(Mat,thersh); _ 
Mat detimg= drawDetections(); _  
vector<Rect> detRect =net->detections; _
delete net; _


## imgdet

Uses darknet to generate more training data from folder with images. 

**Useage**

Key | effect 
--- | --- | 
enter |	correct detection.
tab | incomplete detection.
d | delete image.
other | keep for manual boxing. 


## videt

A tool to grab different cropped image frames from video.  

**Useage**
Key | boxmode | grabmode
--- | --- | --- 
a | add boxes. | 
c | finish adding boxes | clear boxes.
space | select box | save image.