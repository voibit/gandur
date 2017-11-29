/**
 *	@file validate.cpp
 *	validate, https://github.com/voibit/gandur/examples/validate.cpp
 *	@brief Program to validate trained weights
 *	@author Jan-Kristian Mathisen
 *	@author Joachim Lowzow
 */
//TODO: Add multi threading and multi gpu iot speed up.
#include "gandur.hpp"

/**
 * Extends Gandur to add functionality which is not needed
 * for detection
 */
class Valid : public Gandur {
public:
    bool validate(path backupdir, path validfile);
};

int main(int argc, char **argv) {
    path p(argc > 1 ? argv[1] : ".");
    path validlist(argc > 2 ? argv[2] : "");

    auto *net = new Valid();
    net->validate(p, validlist);
    return 0;
}

bool Valid::validate(path backupdir, path validfile) {
    loadVars();

    path cfgname = cfg.netCfg.filename();
    cfgname.replace_extension("");
    float iou_thresh = option_find_float(
            options, (char *) "iou-thresh", 0.5);

    if (validfile == "") {
        validfile = option_find_str(
                options, (char *) "valid", (char *) "../darknet/valid.txt");
    }
    if (!is_regular_file(validfile)) {
        cout << "Valid file not found. please specyfy in conf\n";
        return false;
    }
    if (is_empty(backupdir)) {
        backupdir = string(option_find_str(
                options, (char *) "backup", (char *) "/backup/"));
    }
    if (!exists(backupdir)) {
        cout << "backup dir not found. please specify in conf\n";
        return false;
    }
    vector<path> weights;
    vector<path> imgs;
    /*
    //fill weights vector;
    for(auto &entry : directory_iterator(backupdir)) {
        path filename=entry.path().filename();
        if (filename.extension()==".weights") {

            //check if weights name is the same as cfg name
            string name =filename.string();
            if (name.substr(0, name.find('_')) == cfgname) {
                weights.push_back( (backupdir/filename).string() );
            }
        }
    }
    sort(weights.begin(), weights.end());
    */

    if (is_regular_file(backupdir)) {
        weights.push_back(backupdir);
    } else {
        for (int i = 1000; i < 200000; i += 1000) {
            path wname = cfgname.string() + "_" + std::to_string(i) + ".weights";
            if (exists(backupdir / wname)) {
                weights.push_back(backupdir / wname);
            }
        }
    }

    //fill image vector
    ifstream file(validfile);
    string fname;
    while (std::getline(file, fname)) {
        imgs.emplace_back(fname);
    }
    file.close();

    ofstream ofile(cfgname.replace_extension(".csv"));

    char delim = ',';

    ofile << "Weight" << delim << "IOU" << delim
          << "mAP" + std::to_string((int) (cfg.thresh * 100)) << delim << "mAP 50"
          << delim << "mAP 70" << delim << "mAP 90" << delim << "wrong\n";
    cout << "Weight\tIOU\tmAP\n";

    for (path weight : weights) {

        int wrong = 0;
        int total = 0;
        int correct = 0;
        float avg_iou = 0;
        int c50 = 0;
        int c70 = 0;
        int c90 = 0;

        loadWeights(weight);

        ofstream ofilee(weight.replace_extension("error.csv"));
        ofilee << "file" << delim << "boxnr" << delim
               << "classification[true-proposed]" << endl;

        for (path p : imgs) {
            img = cv::imread(p.string());
            cv::Mat sized = resizeLetterbox(img);

            //Read box from labelfile
            p.replace_extension(".txt");
            int num_labels = 0;
            box_label *truth = read_boxes((char *) p.c_str(), &num_labels);

            /**
             * Do predictions in network
             */
            network_predict(net, bgrToFloat(sized));
            get_region_boxes(l, img.cols, img.rows, net->w, net->h,
                             cfg.thresh, probs, boxes, masks, 0, nullptr, .5, 1);

            //TODO:Add tree detection validation
            //if (l.softmax_tree && nms) do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
            //else if
            if (nms) do_nms_sort(boxes, probs, l.w * l.h * l.n, l.classes, nms);

            //Loop every object in image
            for (int j = 0; j < num_labels; ++j) {

                ++total;
                box t = {truth[j].x, truth[j].y, truth[j].w, truth[j].h};

                std::stringstream ss;

                for (int k = 0; k < l.w * l.h * l.n; ++k) {

                    float iou = box_iou(boxes[k], t);
                    float prob = probs[k][truth[j].id];
                    int maxindex = max_index(probs[k], l.classes);
                    float maxprob = probs[k][maxindex];


                    /**
                     * Check if above detection and iou threshold.
                     */
                    if (maxprob > cfg.thresh && iou > iou_thresh) {
                        ///> Checks that the correct class is detected
                        if (maxindex != truth[j].id) {
                            ss.clear();
                            ++wrong;    ///> Write to errorlis  if not
                            ofilee << p << delim << j << delim
                                   << classNames[truth[j].id] << " " << prob * 100 << "%"
                                   << classNames[maxindex] << " " << maxprob * 100 << "%\n";
                        } else {
                            ++correct;
                            avg_iou += iou;
                        }
                        ///> Add to the different mAP vars of comfident enough
                        if (prob > .5) {
                            ++c50;
                            if (prob > .7) {
                                ++c70;
                                if (prob > .9) ++c90;
                            }
                        }
                    }
                    //TODO: Handle false positives
                    /*extra detections.
                    else if (maxprob > thresh) {
                    }
                    */
                }
            }//NUM LABELS LOOP
        } //img loop

        /**
         * Do percentage calculations and write to file and stdout
         */
        ofilee.close();
        weight.replace_extension("").replace_extension("");
        ofile << weight.filename() << delim << avg_iou * 100 / total
              << delim << 100. * correct / total << delim;
        ofile << 100. * c50 / total << delim << 100. * c70 / total
              << delim << 100. * c90 / total << delim
              << 100. * wrong / total << std::endl;
        cout << weight.filename() << "\t Avg iou:" << avg_iou * 100 / total
             << "\t mAP:" << 100. * correct / total << std::endl;

    } // weights loop
    ofile.close();
}