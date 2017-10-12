//
//  Tracker.cpp
//  Lightwork-Mapper
//
//  Created by Leo Stefansson on 26.9.2017.
//
//

#include "Detector.h"

Detector::Detector() {
    //detectedPattern.generatePattern(0); // Populates bPat.pattern and bPat.patternVector with zeros
}

Detector::~Detector() {
    
}

void Detector::setup(ofVideoGrabber camera) {
    cam = camera;
    mode = DETECTOR_MODE_OFF; // TODO review
    setMinAreaRadius(1);
    setMaxAreaRadius(100);
    setThreshold(15);
    // wait for half a frame before forgetting something (15)
    getTracker().setPersistence(100); // TODO: make an interface for this. Should be 1 for sequential tracking
    // an object can move up to 32 pixels per frame
    getTracker().setMaximumDistance(32);
    getTracker().setSmoothingRate(1.0);
    
    hasFoundFirstContour = false;
    
    // Allocate the thresholded view so that it draws on launch (before calibration starts).
    thresholded.allocate(cam.getWidth(), cam.getHeight(), OF_IMAGE_COLOR);
    thresholded.clear();

}

void Detector::setMode(detector_mode_t m) {
    mode = m;
}

void Detector::update() {
    
    updateViewOnly();
    
    if (mode == DETECTOR_MODE_BINARY) {
        findBinary();
    }
    else if (mode == DETECTOR_MODE_CHASE) {
        findSequential();
    }
}

void Detector::updateViewOnly() {
    // Binary pattern detection
    // Background subtraction
    background.setLearningTime(learningTime);
    background.setThresholdValue(thresholdValue);
    background.update(cam, thresholded);
    
    thresholded.update();
    
    // Get contours
    ofxCv::blur(thresholded, 5); // TODO: do we need this?
    if (mode != DETECTOR_MODE_OFF) {
        findContours(thresholded);
    }
    
    
}

void Detector::findBinary() {
    // Get colour from original frame in contour areas
    if (this->size() <= 0) { cout << "no contour at this moment!" << endl; }
    else {
//        cout << "findBinary size(): " << this->size() << endl;
    }
    // Iterate though all active trackers
    // The size may vary, and the index does not necessarily line up with tracker label (id)
    for (int i = 0; i < this->size(); i++) {
//        ofLogNotice("tracker") << "analyzing tracker at index: " << i << " with label: " << getLabel(i);
        // register the tracker (if it doesn't already exist)
        if (dict.count(getLabel(i)) > 0) {
            //cout << "label already exists, not adding to dictionary" << endl;
        }
        else {
            // Make a new BinaryPattern in the dictionary
            dict[this->getLabel(i)].first = BinaryPattern();
            
            // Store metadata in the BinaryPattern instance (tracker coordinates, physical LED address, etc)
            
        }
        
        //getTracker();
        cv::Rect rect = getBoundingRect(i);
        ofImage img;
        img = cam.getPixels();
        img.crop(rect.x, rect.y, rect.width, rect.height);
        ofPixels pixels = img.getPixels();
        // Pixel format is RGB
        float r = 0;
        float g = 0;
        float b = 0;
        for (int i = 0; i < pixels.getWidth(); i++) {
            for (int j = 0; j < pixels.getHeight(); j++) {
                ofFloatColor col = pixels.getColor(i, j);
                r += col.r;
                g += col.g;
                b += col.b;
            }
        }
        float avgR, avgG, avgB = 0;
        int numPixels = pixels.getWidth()*pixels.getHeight();
        avgR = r/numPixels;
        avgG = g/numPixels;
        avgB = b/numPixels;
        ofFloatColor avgColor = ofFloatColor(avgR, avgG, avgB);
        float brightness = avgColor.getBrightness();
        //cout << "label: " << getLabel(i) <<" brightness: " << brightness << endl;
//        cout << "[" << avgR << ", " << avgG << ", " << avgB << "]," << endl;
        
        // If brightness is above threshold, get the brightest colour
        // Analysis suggests the threshold is around 0.4, I'll use 0.45
        // TODO: Automatically detect threshold value (depends on lighting conditions, background material colour etc.)
        string detectedColor = "";
        int detectedState;
        int dist;
        float brightnessThreshold = 0.65;
        if (brightness >= brightnessThreshold) {
            //                ofLogVerbose("binary") << "Above threshold, check for brightest color" << endl;
            vector<float> colours;
            colours.push_back(avgR);
            colours.push_back(avgG);
            colours.push_back(avgB);
            
            // Get the index of the brightest average colour
            dist = distance(colours.begin(), max_element(colours.begin(), colours.end()));
            //              cout << dist << endl;
            
            // LED binary states:
            // LOW(0) -> RED,
            // HIGH(1) -> BLUE
            // START(2) -> GREEN,
            // OFF(3) -> (off)
            switch (dist) {
                case 0: // LOW
                    detectedColor = "RED";
                    detectedState = 0;
                    break;
                case 1: // START
                    detectedColor = "GREEN";
                    dict[this->getLabel(i)].first.resetBitIndex();
                    detectedState = 2;
                    break;
                case 2: // HIGH
                    detectedColor = "BLUE";
                    detectedState = 1;
                    break;
                default: // OFF
                    ofLogError("binary") << "Brightest colour is not a known colour!" << endl;
            }
        }
        else {
            detectedColor = "BLACK";
            detectedState = 3;
        }
        if (previousState != detectedState && detectedState != 2 && detectedState != 3) {
//            dict[this->getLabel(i)].writeNextBit(detectedState);
            dict[this->getLabel(i)].first.writeNextBit(detectedState);
            dict[this->getLabel(i)].second.set(getCenter(i).x, getCenter(i).y);
            
        }
        previousState = detectedState;
        
        
    }
    // Profit
}
void Detector::findSequential() {
    bool success = false; // Indicate if we successfully mapped an LED on this frame (visible or off-canvas
    
    // We have 1 contour
    if (size() == 1 && !success) {
        ofLogVerbose("tracking") << "Detected one contour, as expected.";
        ofPoint center = ofxCv::toOf(getCenter(0));
        centroids.push_back(center);
        success = true;
    }
    
    // We have more than 1 contour, select the brightest one.
    else if (size() > 1 && !success){ // TODO: review isLedOn vs isMapping()
        ofLogVerbose("tracking") << "num contours: " << ofToString(size());
        int brightestIndex = 0;
        int previousBrightness = 0;
        for(int i = 0; i < size(); i++) {
            int brightness = 0;
            cv::Rect rect = getBoundingRect(i);
            //ofLogNotice("x:" + ofToString(rect.x)+" y:"+ ofToString(rect.y)+" w:" + ofToString(rect.width) + " h:"+ ofToString(rect.height));
            ofImage img;
            img = thresholded;
            img.crop(rect.x, rect.y, rect.width, rect.height);
            ofPixels pixels = img.getPixels();
            
            for (int i = 0; i< pixels.size(); i++) {
                brightness += pixels[i];
            }
            brightness /= pixels.size();
            
            // Check if the brightness is greater than the previous contour brightness
            if (brightness > previousBrightness) {
                brightestIndex = i;
            }
            previousBrightness = brightness;
            success = true;
            //ofLogNotice("Brightness: " + ofToString(brightness));
        }
        ofLogNotice("tracking") << "brightest index: " << ofToString(brightestIndex);
        ofPoint center = ofxCv::toOf(getCenter(brightestIndex));
        centroids.push_back(center);
        hasFoundFirstContour = true;
        //ofLogVerbose("tracking") << "added point, ignored additional points. FrameCount: " << ofToString(ofGetFrameNum())+ " ledIndex: " << animator.ledIndex+(animator.currentStripNum-1)*animator.numLedsPerStrip;
    }
    // Deal with no contours found
    else if (!success && hasFoundFirstContour){
        ofLogVerbose("tracking") << "NO CONTOUR FOUND!!!";
        
        // No point detected, create fake point
        ofPoint fakePoint;
        fakePoint.set(0, 0);
        centroids.push_back(fakePoint);
        success = true;
        //ofLogVerbose("tracking") << "CREATING FAKE POINT                     at frame: " << ofToString(ofGetFrameNum()) << " ledIndex " << animator.ledIndex+(animator.currentStripNum-1)*animator.numLedsPerStrip;
    }
    
    if(success) {
        hasFoundFirstContour = true;

    }
    
}
