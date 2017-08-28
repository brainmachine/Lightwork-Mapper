//
//  BinaryPattern.cpp
//  Lightwork-Mapper
//
//  Created by Leo Stefansson on 28.8.2017.
//
//

#include "BinaryPattern.h"
#include <iostream>

using namespace std;

void BinaryPattern::createPattern(int num) {
    cout << binary(num);
    
    
};


int BinaryPattern::binary(int num) {
    int remainder;
    
    if(num <= 1) {
        cout << num;
        return;
    }
    
    remainder = num%2;
    binary(num >> 1);
    cout << remainder;
    return remainder;
}
