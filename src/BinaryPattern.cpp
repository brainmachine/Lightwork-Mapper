//
//  BinaryPattern.cpp
//  Lightwork-Mapper
//
//  Created by Leo Stefansson on 28.8.2017.
//
//

#include "BinaryPattern.h"
#include <iostream>
#include <sstream> 

#include <bitset>




using namespace std;

std::string BinaryPattern::createPattern(int num) {
    std::string s = std::bitset< 10 >( num ).to_string(); // string conversion
    return s;
};

