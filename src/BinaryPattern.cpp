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

void BinaryPattern::generatePattern(int num) {
    // Generate a bitset with 10 bits. If the bit sequence is shorter it will append zeros to make the length 10.
    std::string s = std::bitset< 10 >( num).to_string(); // string conversion
    // Store bitstrings in patterns vector
    patternString = s;
};

