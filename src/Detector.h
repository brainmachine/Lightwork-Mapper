//
//  Tracker.hpp
//  Lightwork-Mapper
//
//  Created by Leo Stefansson on 26.9.2017.
//
//

#ifndef Tracker_h
#define Tracker_h

#include <stdio.h>
#include "ofxCv.h"
#include "BinaryPattern.h"
#include "LED.h"
#include "Scope.h"
#include <map>

enum detector_mode_t {DETECTOR_MODE_CHASE, DETECTOR_MODE_BINARY, DETECTOR_MODE_OFF};

class Detector : public ofxCv::ContourFinder {
    
public:
    Detector();
    ~Detector();
    
    // Background subtraction
    ofxCv::RunningBackground        background;        // Background subtraction class with running average
    ofImage                         thresholded;       // Binary threshold image
    
    float                   threshold;          // Brightness threshold for contour detection
    vector <ofPoint>        centroids;          // Stores the contour area centers.
    bool                    hasFoundFirstContour;   // Avoid registering 'fake' points before the first detection
    ofParameter<float>      learningTime;
    ofParameter<float>      thresholdValue; // Camera brightness threshold
    
    // Binary Detection Thresholds
    ofParameter<float>      binaryBrightnessThreshold; 
    ofParameter<float>      blueGreenDistanceThreshold;
    
    ofVideoGrabber          cam;
    
    void setup(ofVideoGrabber camera);
    void setMode(detector_mode_t m);
    detector_mode_t mode;
    
    void update();
    void updateViewOnly();
    void findBinary();
    void findSequential();
    
    map <unsigned int, pair<BinaryPattern, ofPoint>>            dict; // Key: tracker label, value: binary pattern
    
    
    
private:
    
    
};

#endif /* Tracker_h */
