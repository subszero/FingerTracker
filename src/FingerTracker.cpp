#include "FingerTracker.h"


//--------------------------------------------------------------
void FingerTracker::setup() {
    //Setup OF
    ofSetFrameRate(60);
    ofSetWindowTitle("FingerTracker");
    
	//Setup Kinect
	kinect.init();
	kinect.setVerbose(true);
	kinect.open();
    angle = 0;
	kinect.setCameraTiltAngle(angle);
    
    //Setup Images
    grayImage.allocate(kinect.width, kinect.height);
    colorImage.allocate(kinect.width, kinect.height);
    depthFrameRaw =  new cv::Mat(480, 640, CV_16UC1);
    x = new cv::Mat(480, 640, CV_32FC1);
    y = new cv::Mat(480, 640, CV_32FC1);
    z = new cv::Mat(480, 640, CV_32FC1);
    debugFrame = new cv::Mat1f(480, 640);
    rMin = 25;
    zMin = 0.0f;
    zMax = 0.75f;
    
    //Setup detection
    nearThreshold = 255;
    farThreshold = 200;
    mirror = false;
    
    //Setup GUI
    gui.setup();
    gui.config->gridSize.x = 200;
    gui.addTitle("Kinect Settings");
    gui.addSlider("Tilt Angle", angle, -30, 30);
    gui.addToggle("Mirror", mirror);
    gui.addTitle("Depth Threshold");
    gui.addSlider("Near Distance", nearThreshold, 0, 255);
    gui.addSlider("Far Distance", farThreshold, 255, 0);
    gui.setDefaultKeys(true);
    gui.loadFromXML();
    gui.show();
}

//--------------------------------------------------------------
void FingerTracker::update() {
	ofBackground(55, 50, 50);
    kinect.setCameraTiltAngle(angle);
	kinect.update();
    
	if(kinect.isFrameNew())	{
        depthFrameRawData = kinect.getRawDepthPixels();
        grayImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
        colorImage.setFromPixels(kinect.getPixels(), kinect.width, kinect.height);
        unproject(depthFrameRawData, (float*)x->data, (float*)y->data, (float*)z->data);
        //debugFrame = z * 0.1;
        fingerTips = detectFingers(*z, zMin, zMax);
    }
}

//--------------------------------------------------------------
void unproject(unsigned short *depth, float *x, float *y, float *z) {
    int u,v;
	const float f = 500.0f;
	const float u0 = 320.0f;
	const float v0 = 240.0f;
	float zCurrent;
    
	// TODO calibration
    
	for (int i=0; i<640*480; i++) {
		u = i % 640;
		v = i / 640;
		zCurrent = 1.0f / (-0.00307110156374373f * depth[i] + 3.33094951605675f);
		if (z != NULL) {
			z[i] = zCurrent;
		}
		if (x != NULL) {
			x[i] = (u - u0) * zCurrent / f;
		}
		if (y != NULL) {
			y[i] = (v - v0) * zCurrent / f;
		}
	}
}

//--------------------------------------------------------------
vector<cv::Point2i> detectFingers(cv::Mat1f z, float zMin, float zMax) {
    vector<cv::Point2i> fingers;
    
    cv::Mat handMask = z < zMax & z > zMin;
    std::vector< std::vector<cv::Point> > contours;
    
    cv::findContours(handMask, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
    if(contours.size()) {
        for (int i=0; i<contours.size(); i++) {
            vector<cv::Point> contour = contours[i];
            cv::Mat contourMat(contour);
            double area = cv::contourArea(contourMat);
            
            if(area > 3000) {
                cv::Scalar center = cv::mean(contourMat);
                cv::Point centerPoint(center.val[0], center.val[1]);
                vector<cv::Point> approxCurve;
                cv::approxPolyDP(contourMat, approxCurve, 20, true);
                vector<int> hull;
                cv::Mat temp(approxCurve);
                cv::convexHull(temp, hull);
                
                int upper=640, lower=0;
                for(int j=0;j<hull.size();j++) {
                    int idx = hull[j];
					if (approxCurve[idx].y < upper) upper = approxCurve[idx].y;
					if (approxCurve[idx].y > lower) lower = approxCurve[idx].y;
                }
                
                float cutoff = lower - (lower-upper)*0.1f;
            
                for (int j=0; j<hull.size(); j++) {
					int idx = hull[j];
					int pdx = idx == 0 ? approxCurve.size() - 1 : idx - 1;
					int sdx = idx == approxCurve.size() - 1 ? 0 : idx + 1;
                    
                    cv::Point v1 = approxCurve[sdx] - approxCurve[idx];
                    cv::Point v2 = approxCurve[pdx] - approxCurve[idx];
                    
					float angle = acos( (v1.x*v2.x + v1.y*v2.y) / (norm(v1) * norm(v2)) );
                    
					if (angle < 1 && approxCurve[idx].y < cutoff) {
						int u = approxCurve[idx].x;
						int v = approxCurve[idx].y;
            
						fingers.push_back(cv::Point2i(u,v));
					}
				}
            }
        }
    }
    return fingers;
}

//--------------------------------------------------------------
void FingerTracker::draw() {
    gui.draw();
	ofSetColor(255, 255, 255);
    colorImage.draw(550, 100, 160, 120);
    grayImage.draw(250, 250, kinect.width, kinect.height);
	ofSetColor(255, 255, 255);
}

//--------------------------------------------------------------
void FingerTracker::exit() {
	kinect.setCameraTiltAngle(0);
	kinect.close();
    delete depthFrameRaw;
    delete x;
    delete y;
    delete z;
    delete debugFrame;
}

//--------------------------------------------------------------
void FingerTracker::keyPressed (int key) {
	switch (key) {
		case '>':
		case '.':
			farThreshold ++;
			if (farThreshold > 255) farThreshold = 255;
			break;
		case '<':		
		case ',':		
			farThreshold --;
			if (farThreshold < 0) farThreshold = 0;
			break;
			
		case '+':
		case '=':
			nearThreshold ++;
			if (nearThreshold > 255) nearThreshold = 255;
			break;
		case '-':		
			nearThreshold --;
			if (nearThreshold < 0) nearThreshold = 0;
			break;

		case OF_KEY_UP:
			angle++;
			if(angle>30) angle=30;
			kinect.setCameraTiltAngle(angle);
			break;

		case OF_KEY_DOWN:
			angle--;
			if(angle<-30) angle=-30;
			kinect.setCameraTiltAngle(angle);
			break;
	}
}

//--------------------------------------------------------------
void FingerTracker::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void FingerTracker::mouseDragged(int x, int y, int button) {}

//--------------------------------------------------------------
void FingerTracker::mousePressed(int x, int y, int button) {}

//--------------------------------------------------------------
void FingerTracker::mouseReleased(int x, int y, int button) {}

//--------------------------------------------------------------
void FingerTracker::windowResized(int w, int h) {}

