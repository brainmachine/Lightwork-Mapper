//
//  LED.cpp
//  Lightwork-Mapper
//
//  Created by Leo Stefansson on 3.10.2017.
//
//

#include "LED.h"


LED::LED() {
    color = ofColor(0, 0, 0);
    address = 0;
    coord = ofPoint(0, 0);
    hasFoundMatch = false;
    
    lastCoord = ofPoint(0,0);
    validationCount = 0;
    validationThreshold = 20; // TODO: Magic
    distanceThreshold = 1.5; // TODO: Tweak this Magic Number
    
//    binaryPattern = BinaryPattern();
}

LED::~LED() {
    
}

void LED::setColor(ofColor col) {
    color = col;
}

void LED::setAddress(int addr) {
    address = addr;
}

void LED::setBinaryPattern(BinaryPattern pat) {
    binaryPattern = pat;
}

void LED::setCoord(ofPoint c) {
    coord = c;
    validateCoord(c);
    
}

void LED::validateCoord(ofPoint c) {
    if (coord == ofPoint(0, 0) && lastCoord == ofPoint(0,0)) {
        // Coord has not been set, don't validate
        return;
    }
    
    float dist = coord.distance(lastCoord);
// --->
    //cout << dist << ", ";
// --->
    if (dist < distanceThreshold) {
        validationCount++;
        if (validationCount >= validationThreshold) {
            hasFoundMatch = true;
        }
        
    }
    else {
        hasFoundMatch = false;
    }
    
    lastCoord = coord;
    
}
