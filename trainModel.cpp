/**##############################################################
## MODULE: trainModel.cpp
## VERSION: 1.0 
## SINCE 2013-04-24
## AUTHOR Jimmy Lin (u5223173) - u5223173@uds.anu.edu.au  
## DESCRIPTION: 
##      
#################################################################
## Edited by MacVim
## Class Info auto-generated by Snippet 
################################################################*/
// c++ standard headers
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>

// eigen matrix library headers
#include "Eigen/Core"

// opencv library headers
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"

// darwin library headers
#include "drwnBase.h"
#include "drwnIO.h"
#include "drwnML.h"
#include "drwnVision.h"
#include "features.h"
#include "mexImageCRF.h"
using namespace std;
using namespace Eigen;

// usage ---------------------------------------------------------------------

// copied from Stephen Gould's trainCOMP3130Model.cpp 2013 version
void usage() {
    cerr << DRWN_USAGE_HEADER << endl;
    cerr << "USAGE: ./trainModel [OPTIONS] <imgDir> <lblFile>\n";
    cerr << "OPTIONS:\n"
         << "  -o <model>        :: output model\n"
         << "  -x                :: visualize\n"
         << DRWN_STANDARD_OPTIONS_USAGE
	 << endl;
}

// parseLabel ----------------------------------------------------------------

/* Form a hash map between the filenames and integer vectors representing the 
 * salient rectangle boundaries in the corresponding images. 
 */
map< string, vector<int> > parseLabel (const char * labelFileName) {
    map< string, vector<int> > fileLabelPairs; // MAP FROM FILENAME TO RECTANGLE
    string line;
    ifstream labelFile ;
    labelFile.open(labelFileName);
    while ( !labelFile.eof() ){
        unsigned int pos;
        string imagePackage;
        string imageFilename;
        int widthOfImage, heightOfImage;
        vector<int> posRectangle;
        posRectangle.resize(4);
        // restore every line of the file
        getline(labelFile, line); 
        if (line.find( ".jpg") != std::string::npos) {
            // PARSE for the first line 
            // - which stores the image name and package
            pos = line.find("\\");
            imageFilename = line.substr(pos+1); // get filename
            imagePackage = line.substr(0, pos); // get package number
            imageFilename = imageFilename.substr(0, imageFilename.size() - 1); // truncate string
            // test for printing
            //cout << "imageFilename:" << imageFilename  << endl;
            //cout << imageFilename.size() << endl;

            // PARSE for the SECOND line
            //  width and height of that training image
            //  use c lib's string 2 integer
            getline(labelFile, line);
            pos = line.find(" ");
            widthOfImage = atoi(line.substr(0,pos).c_str()); 
            heightOfImage = atoi(line.substr(pos+1).c_str()); 
            // print for test
            //cout << "Width:" << widthOfImage << "\t" << "Height:" << heightOfImage << endl;

            // PARSE for the third line
            // three saliency rectangles, each with four parameter
            getline(labelFile, line);
            string temp;
            for (int i = 0 ; i < 3; i ++) {
                pos = line.find(";");
                temp = line.substr(0, pos);
                sscanf(temp.c_str(), "%d %d %d %d", &posRectangle[0],
                        &posRectangle[1], &posRectangle[2], &posRectangle[3]);
                line = line.substr(pos+1);
                // printing for test
                //printf("Left: %3d \t Top: %4d \t Right: %4d \t Bottom: %4d \n",
                        //posRectangle[0], posRectangle[1], posRectangle[2], posRectangle[3]);
                if (i == 1) { // we choose second data here.
                    fileLabelPairs[imageFilename] = posRectangle;
                }
            }
            //cout << endl;
        }
    }
    labelFile.close();
    
    return fileLabelPairs;
}
// main ----------------------------------------------------------------------

int main (int argc, char * argv[]) {
    // what way are we building up a default classifier?
    // Set default value for optional command line arguments.
    const char *modelFile = NULL;
    bool bVisualize = false;

    DRWN_BEGIN_CMDLINE_PROCESSING(argc, argv)
        DRWN_CMDLINE_STR_OPTION("-o", modelFile)
        DRWN_CMDLINE_BOOL_OPTION("-x", bVisualize)
    DRWN_END_CMDLINE_PROCESSING(usage());

    // Check for the correct number of required arguments
    if (DRWN_CMDLINE_ARGC != 6) {
        usage();
        return -1;
    }

    /* Check that the image directory and labels directory exist. All
     * images with a ".jpg" extension will be used for training the
     * model. It is assumed that the labels directory contains files
     * with the same base as the image directory, but with extension
     * ".txt". 
     */
    const char *imgDir = DRWN_CMDLINE_ARGV[0]; // directory restores original images
    const char *mscDir = DRWN_CMDLINE_ARGV[1]; // directory restores multiscale contrast feature map
    const char *cshDir = DRWN_CMDLINE_ARGV[2]; // directory restores center surround histogram feature map
    const char *csdDir = DRWN_CMDLINE_ARGV[3]; // directory restores color spatial distribution feature map
    const char *lblFile = DRWN_CMDLINE_ARGV[4]; // a single text file with ground truth rectangle
    const char *outputDir = DRWN_CMDLINE_ARGV[5]; // directory for resulting images
    // Check for existence of the directory containing orginal images
    DRWN_ASSERT_MSG(drwnDirExists(imgDir), "image directory " << imgDir << " does not exist");
    // Check for existence of the ground truth data
    //DRWN_ASSERT_MSG(drwnDirExists(lblFile), "labels directory " << lblFile << " does not exist");

    // Get a list of images from the image directory.
    vector<string> baseNames = drwnDirectoryListing(imgDir, ".jpg", false, false);
    DRWN_LOG_MESSAGE("Loading " << baseNames.size() << " images and labels...");

    // Build a dataset by loading images and labels. For each image,
    // find the salient area using the labels and then compute the set of features
    // that determine this saliency
    drwnClassifierDataset dataset;
    //  MAP FROM FILENAME TO RECTANGLE
    map< string, vector<int> > fileLabelPairs = parseLabel(lblFile);
    int left, top, right, bottom;
    vector<int> tempRectangle;

    for (unsigned i = 0; i < baseNames.size(); i++) {
        String processedImage = baseNames[i] + ".jpg";
        DRWN_LOG_STATUS("...processing image " << baseNames[i]);
        // read the image and draw the rectangle of labels of training data
        cv::Mat img = cv::imread(string(imgDir) + DRWN_DIRSEP + processedImage);
        cv::Mat msc = cv::imread(string(mscDir) + DRWN_DIRSEP + processedImage);
        cv::Mat csh = cv::imread(string(cshDir) + DRWN_DIRSEP + processedImage);
        cv::Mat csd = cv::imread(string(csdDir) + DRWN_DIRSEP + processedImage);
        // ground truth label
        tempRectangle = fileLabelPairs.find(processedImage)->second ;
        left =  tempRectangle [0];
        top = tempRectangle [1];
        right = tempRectangle [2];
        bottom = tempRectangle[3];

        // show the image and feature maps 
        if (bVisualize) { // draw the current image comparison
            //drwnDrawRegionBoundaries and drwnShowDebuggingImage use OpenCV 1.0 C API
            IplImage cvimg = (IplImage)img;
            IplImage *canvas = cvCloneImage(&cvimg);
            drwnShowDebuggingImage(canvas, "image", false);
            cvReleaseImage(&canvas);
        }
        // get unary potential and combine them by pre-computed parameters 
        vector< cv::Mat > unary(2);
        unary[0] = cv::Mat(img.rows, img.cols, CV_64F);
        unary[1] = cv::Mat(img.rows, img.cols, CV_64F);
        double grayscale;
        for (int y = 0; y < img.rows; y ++) {
            for (int x = 0 ; x < img.cols; x ++) {
                grayscale = 0.22*msc.at<Vec3b>(y,x).val[0] + 0.54*csh.at<Vec3b>(y,x).val[0] + 0.24*csd.at<Vec3b>(y,x).val[0];
                unary[0].at<double>(y,x) = grayscale / 255.0;
                unary[1].at<double>(y,x) = 1 - unary[0].at<double>(y,x);
            }
        }
        // compute binary mask of each pixel
        const double lambda = 2;
        cv::Mat binaryMask = mexFunction(img, unary, lambda);

        // interpret the binary mask to two-color image
        cv::Mat pres(img.rows, img.cols, CV_8UC3);
        for (int y = 0 ; y < img.rows; y ++) {
            for (int x = 0 ; x < img.cols; x ++) {
                int tempSaliency = binaryMask.at<short>(y,x)*255>125?255:0;
                pres.at<cv::Vec3b>(y,x) = cv::Vec3b(tempSaliency, tempSaliency, tempSaliency);
            }
        }
        // present the derived binary mask by white-black image
        IplImage pcvimg = (IplImage) pres;
        IplImage *present = cvCloneImage(&pcvimg);
        cv::imwrite(string(outputDir) + baseNames[i] + ".jpg", pres);
        if (bVisualize) { // draw the processed feature map and display it on the screen
            drwnShowDebuggingImage(present, "Composed Graph", false);
            cvReleaseImage(&present);
        }

    }

    // Clean up by freeing memory and printing profile information.
    cvDestroyAllWindows();
    drwnCodeProfiler::print();
    return 0;
}
