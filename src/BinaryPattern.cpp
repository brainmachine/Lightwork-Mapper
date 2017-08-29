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
    std::string s = std::bitset< 10 >( num ).to_string(); // string conversion
    
    // Store bitstrings in pattern string
    pattern = s;
    
    // Convert to int vector and store internally
    patternVector = convertStringToIntVector(pattern);
    
};

vector <int> BinaryPattern::convertStringToIntVector(string pattern) {
    // Convert binary string to vector of ints
    std::vector<int> ints;
    ints.reserve(pattern.size()); //to save on memory reallocations
    
    std::transform(std::begin(pattern), std::end(pattern), std::back_inserter(ints),
                   [](char c) {
                       //cout << c - '0';
                       return c - '0'; // Distance between c and the string '0'
                   }
                   );
    return ints;
}



