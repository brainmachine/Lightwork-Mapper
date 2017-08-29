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
//    ~BinaryPattern();
    void generatePatterns(int num);
    std::vector <std::string> patterns; // For Storing Binary Patterns
    
private:
    string patternString; // Stores a single binary pattern
    int frameNum; 
    
};

// Constructor
BinaryPattern::BinaryPattern(void) {
    frameNum = 0;
}
