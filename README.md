
# Gandur

C++ wrapper class for darknet.
based upon prabindhs "arapaho". [https://github.com/prabindh/darknet]

**Useage**

```c++
Gandur *net= new Gandur();  
net->Detect(Mat,thersh);  
Mat detimg= drawDetections();  
vector<Rect> detRect =net->detections;  
delete net;
```  


## imgdet

Uses darknet to generate more training data from folder with images. 

**Useage**

./imdet folder

Key | effect 
--- | --- | 
enter |	correct detection.
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