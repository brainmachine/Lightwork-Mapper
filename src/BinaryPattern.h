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
    ~BinaryPattern(); // Destructor
    
    void generatePattern(int num);
    
    //std::vector <std::string> patterns; // For Storing Binary Patterns
    
    enum ledState_t {LOW, HIGH, START}; // LED binary state. START -> GREEN, HIGH -> BLUE, LOW -> RED
    ledState_t state;
    
private:
    string patternString; // Stores a single binary pattern
    int frameNum;
};

// Constructor
BinaryPattern::BinaryPattern(void) {
    frameNum = 0;
    state = LOW;
}
