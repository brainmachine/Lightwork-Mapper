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

void BinaryPattern::generatePatterns(int num) {
    
    // Generate a set of patterns
    int offset = 33; // Binary sequence 0000000000 is not meaningful, lets start with a higher number
    for (int i = offset; i < num+offset; i++) {
        // Generate a bitset with 10 bits. If the bit sequence is shorter it will append zeros to make the length 10.
        std::string s = std::bitset< 10 >( i ).to_string(); // string conversion
        // Store bitstrings in patterns vector
        patterns.push_back(s);
    }
   
};

