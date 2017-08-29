//
//  BinaryPattern.hpp
//  Lightwork-Mapper
//
//  Created by Leo Stefansson on 28.8.2017.
//
//

#ifndef BinaryPattern_h
#define BinaryPattern_h

#include <stdio.h>
#include <string>
#include <vector>


#endif /* BinaryPattern_h */

using namespace std;

class BinaryPattern {
    
    
public:
    BinaryPattern(); // Constructor
    //~BinaryPattern(); // Destructor
    
    void generatePattern(int num);
    
    string pattern; // Stores a single binary pattern
    vector <int> patternVector; // Stores binary pattern as vector of ints
    
    //std::vector <std::string> patterns; // For Storing Binary Patterns
    
    enum ledState_t {LOW, HIGH, START}; // LED binary state. START -> GREEN, HIGH -> BLUE, LOW -> RED
    ledState_t state;

    int frameNum;
    
private:
    vector <int> convertStringToIntVector(string pattern);
};

// Constructor
BinaryPattern::BinaryPattern(void) {
    frameNum = 0;
    state = LOW;
}
