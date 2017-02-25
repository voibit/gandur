
#include "arapaho.hpp"

ArapahoV2::ArapahoV2()
{
        boxes = 0;
        probs = 0;    
}
    
ArapahoV2::~ArapahoV2()
{
    if(boxes) 
        free(boxes);
    if(probs)
        free_ptrs((void **)probs, l.w*l.h*l.n);
    boxes = 0;
    probs = 0;
}
    
bool ArapahoV2::Setup(
            ArapahoV2Params & p,
            int & expectedWidth,
            int & expectedHeight
                     )
{
    expectedHeight = expectedWidth = 0;
    
    if(!p.datacfg)
    {
        printf("No data configuration file specified!\n");
        return false;
    }    
    
    list *options = read_data_cfg(p.datacfg);
    char *name_list = option_find_str(options, "names", "data/names.list");
    char **names = get_labels(name_list);
    int j;
    bool ret = false;
        
    if(!p.cfgfile)
    {
        printf("No cfg file specified!\n");
        return false;
    }

    nms = p.nms;
    net = parse_network_cfg(p.cfgfile);
    
    if(!p.weightfile)
    {
        printf("No weights file specified!\n");
        return false;
    }    
    load_weights(&net, p.weightfile);
    
    set_batch_network(&net, 1);     
    l = net.layers[net.n-1];
    
    expectedHeight = net.h;
    expectedWidth = net.w;
    
    boxes = (box*)calloc(l.w*l.h*l.n, sizeof(box));
    probs = (float**)calloc(l.w*l.h*l.n, sizeof(float *));
    
    if(!boxes || !probs)
    {
        printf("Error allocating boxes/probs, %x/%x !\n", boxes, probs);
        goto clean_exit;
    }

    for(j = 0; j < l.w*l.h*l.n; ++j) 
    {
        probs[j] = (float*)calloc(l.classes + 1, sizeof(float));
        if(!probs[j])
        {
            printf("Error allocating probs[%d]!\n", j);            
            goto clean_exit;
        }
    }
    ret = true;
    return ret;
    
clean_exit:        
    if(boxes) 
        free(boxes);
    if(probs)
        free_ptrs((void **)probs, l.w*l.h*l.n);
    
    return ret;
}

bool ArapahoV2::Detect(
            ArapahoV2ImageBuff & imageBuff, 
            float thresh, 
            float hier_thresh,
            int   maximumClasses,
            int & objectCount)
{
    int i;
        
    objectCount = 0;
    threshold = thresh;
    maxClasses = maximumClasses;
    
    if(!imageBuff.bgr || imageBuff.w != net.w || imageBuff.h != net.h)
    {
        printf("Error in imageBuff! [Checked bgr = %d, w = %d, h = %d]\n", !imageBuff.bgr, imageBuff.w != net.w, imageBuff.h != net.h);
        return false;        
    }        
    
    // Get the image to suit darknet
    image inputImage;
    inputImage.data = imageBuff.bgr;
    inputImage.h = imageBuff.h;
    inputImage.w = imageBuff.w;
    inputImage.c = imageBuff.channels;
    
    if (inputImage.h != net.h || inputImage.w != net.w)
    {
        inputImage = resize_image(inputImage, net.w, net.h);
    }
    // Predict
    network_predict(net, inputImage.data);
    get_region_boxes(l, 1, 1, thresh, probs, boxes, 0, 0, hier_thresh);
    
    if (l.softmax_tree && nms)
        do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
    else if (nms) 
        do_nms_sort(boxes, probs, l.w*l.h*l.n, l.classes, nms);
    
    for(i = 0; i < (l.w*l.h*l.n); ++i){
        int class1 = max_index(probs[i], maxClasses);
        float prob = probs[i][class1];
        if(prob > thresh){
            objectCount ++;
        }
    }

    return true;
}
    
bool ArapahoV2::GetBoxes(box* outBoxes, int boxCount)
{
    
    int count = 0;
    int i;
    
    if(!boxes || !probs)
    {
        printf("Error NULL boxes/probs, %x/%x !\n", boxes, probs);
        return false;
    }        
    for(i = 0; i < (l.w*l.h*l.n); ++i)
    {
        int class1 = max_index(probs[i], maxClasses);
        float prob = probs[i][class1];
        if(prob > threshold && count < boxCount)
        {
            outBoxes[count ++] = boxes[i];
        }
    }
    
    return true;
}
