//
//  BinaryPattern.cpp
//  Lightwork-Mapper
//
//  Created by Leo Stefansson on 28.8.2017.
//
//
#pragma once

#include "BinaryPattern.h"
#include <iostream>

using namespace std;

void BinaryPattern::createPattern() {
    int num = 10;
    cout << binary(num);
    
    
};


int BinaryPattern::binary(int number) {
    int remainder;
    
    if(number <= 1) {
        cout << number;
        return;
    }
    
    remainder = number%2;
    binary(number >> 1);
    cout << remainder;
    return remainder;
}
