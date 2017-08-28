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

void BinaryPattern::createPattern(int num) {
    std::string s = std::bitset< 10 >( num ).to_string(); // string conversion
    cout << s;
 
};
