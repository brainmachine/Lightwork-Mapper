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
    setMaxAreaRadius(50);
    setThreshold(15);
    // wait for half a frame before forgetting something (15)
    getTracker().setPersistence(200); // TODO: make an interface for this. Should be 1 for sequential tracking
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
   // if (this->size() <= 0) { cout << "no contour at this moment!" << endl; }

    // Iterate though all active trackers
    // The size may vary, and the index does not necessarily line up with tracker label (id)

    for (int i = 0; i < this->size(); i++) {
        // register the tracker (if it doesn't already exist)
        if (dict.count(getLabel(i)) > 0) {
            //cout << "label already exists, not adding to dictionary" << endl;
        }
        else {
            // Make a new BinaryPattern in the dictionary
            dict[this->getLabel(i)].first = BinaryPattern();
            dict[this->getLabel(i)].second.set(getCenter(i).x, getCenter(i).y);
        }
        // Update position
        dict[this->getLabel(i)].second.set(getCenter(i).x, getCenter(i).y);
        
        // Get brightness and average colour in region
        ofLogVerbose("detector") << "Analysizing brightness for tracker at index: " << i << " with label: " << getLabel(i);
        cv::Rect rect = getBoundingRect(i);
        ofImage img;
        img = cam.getPixels();
        img.crop(rect.x, rect.y, rect.width, rect.height);
        ofPixels pixels = img.getPixels();
        // Pixel format is RGB
        float r = 0;
        float g = 0;
        float b = 0;
        for (int j = 0; j < pixels.getWidth(); j++) {
            for (int k = 0; k < pixels.getHeight(); k++) {
                ofFloatColor col = pixels.getColor(j, k);
                //if (col.getSaturation() > 0.2) {
                    r += col.r;
                    g += col.g;
                    b += col.b;
                //}
            }
        }
        float avgR, avgG, avgB = 0;
        int numPixels = pixels.getWidth()*pixels.getHeight();
        avgR = r/numPixels;
        avgG = g/numPixels;
        avgB = b/numPixels;
        

        
        ofFloatColor avgColor = ofFloatColor(avgR, avgG, avgB);
        float brightness = avgColor.getBrightness();
        ofLogVerbose("detector") << "Brightness: " << brightness;
// ---->
        if (i == 2) {
            cout << "[" << avgR << ", " << avgG << ", " << avgB << ", " << brightness << "]," << endl;
            //cout << brightness << endl;
        }
// ---->
        
        // If brightness is above threshold, get the brightest colour
        int dist;
        
        float brightnessThreshold = 0.72;
        if (brightness >= brightnessThreshold) {
            vector<float> colours;
            colours.push_back(avgR);
            colours.push_back(avgG);
            colours.push_back(avgB);
        
            // Get the index of the brightest average colour
            dist = distance(colours.begin(), max_element(colours.begin(), colours.end()));
            
            // Note: the above state DO NOT reflect the 'dist' used here
            if (dist == 1 || dist == 2) {
                // Check if Blue and Green are too close to each other
                // When we light up green, it also brings up the blue levels
                // We want to make sure that the distance between green and blue is sufficient
                // If the distance is greater than the threshold we assume it's blue, if it's below it's green
                float colorDistanceThreshold = 0.1;
                float colorDistance = std::abs(avgB-avgG);
// ---->
                //cout << colorDistance << ", ";
// ---->
                if (colorDistance <= colorDistanceThreshold) {
                    dist = 1; // Set to green
                //    cout << "Distance between green and blue is lower than threshold. We should detect GREEN" << endl;
                }
                else {
                    dist = 2; // Set to blue
                //    cout << "Distance between green and blue is greater than threshold. We should detect BLUE" << endl;
                }
            }
            
            // LED binary states:
            // LOW(0) -> RED,
            // HIGH(1) -> BLUE
            // START(2) -> GREEN,
            // OFF(3) -> (off)
            
            switch (dist) {
                case 0: // LOW (RED)
                    dict[getLabel(i)].first.detectedState = 0;
                    break;
                case 1: // START (GREEN)
                    dict[this->getLabel(i)].first.resetBitIndex();
                    dict[getLabel(i)].first.detectedState = 2;
                    break;
                case 2: // HIGH (BLUE)
                    dict[getLabel(i)].first.detectedState = 1;
                    break;
                default: // OFF
                    ofLogError("binary") << "Brightest colour is not a known colour!" << endl;
            }
        }
        else {
            // In between bytes (BLACK)
            dict[getLabel(i)].first.detectedState = 3;
        }
        
        // Write detected byte to dictionary(Label:BinaryPattern)
        if (dict[getLabel(i)].first.previousState != dict[getLabel(i)].first.detectedState && dict[getLabel(i)].first.detectedState != 2 && dict[getLabel(i)].first.detectedState != 3) {
            dict[getLabel(i)].first.writeNextBit(dict[getLabel(i)].first.detectedState);
        }
        
        dict[getLabel(i)].first.previousState = dict[getLabel(i)].first.detectedState;
    }
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
            // Copy the image region of interest
            cv::Rect rect = getBoundingRect(i);
            ofImage img;
            img = thresholded;
            img.crop(rect.x, rect.y, rect.width, rect.height);
            ofPixels pixels = img.getPixels();
            
            // Calculate brightness of region
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
        }
        
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
