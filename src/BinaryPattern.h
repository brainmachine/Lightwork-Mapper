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


#endif /* BinaryPattern_h */

class BinaryPattern {
    
public:
//    BinaryPattern();
//    ~BinaryPattern();
    
    std::string createPattern(int num);

private:
    int binary(int num);
    std::string patternString; // Stores a single binary pattern
    
};
