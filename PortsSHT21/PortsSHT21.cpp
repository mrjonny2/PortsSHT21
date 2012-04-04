// Port library interface to SHT21 sensors connected via "something like I2C"
// 2009-02-16 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: PortsSHT21.cpp 5402 2010-04-30 19:24:52Z jcw $

// rewritten in C++ using the SENSIRION SHTxx Sample Code Application Note

#include <Ports.h>
#include "PortsSHT21.h"
#include <avr/pgmspace.h>
#include <Arduino.h>

enum {
    MEASURE_TEMP = 0xE3,
    MEASURE_HUMI = 0xE5,
};

// idle line state is with data as input pulled high, and clock as output low

void SHT21::clock(uint8_t x) const {
    delayMicroseconds(2);
    digiWrite2(x);
    delayMicroseconds(5);
}

void SHT21::release() const {
    mode(INPUT);
    digiWrite(1);
}

uint8_t SHT21::writeByte(uint8_t value) const {
    mode(OUTPUT);
    for (uint8_t i = 0x80; i != 0; i >>= 1) {
        digiWrite(value & i);
        clock(1);
        clock(0);
    }
    release();
    clock(1);
    uint8_t error = digiRead();
    clock(0);
    
    return error;
}

uint8_t SHT21::readByte(uint8_t ack) const {
    uint8_t value = 0;
    for (uint8_t i = 0x80; i != 0; i >>= 1) {
        clock(1);
        if (digiRead())
            value |= i;
        clock(0);
    }
    mode(OUTPUT);
    digiWrite(!ack);
    clock(1);
    clock(0);
    release();
    
    return value;
}

void SHT21::start() const {
    clock(0);
    mode(OUTPUT);
    digiWrite(1);
    
    clock(1); 
    digiWrite(0); 
    clock(0);   
    clock(1); 
    digiWrite(1);      
    clock(0);
    release();
}

void SHT21::connReset() const {
    mode2(OUTPUT);
    clock(0);
    mode(OUTPUT);
    digiWrite(1);
    for (uint8_t i = 0; i < 9; ++i) {
        clock(1);
        clock(0);
    }
    start();
}

uint8_t SHT21::measure(uint8_t type, void (*delayFun)()) {
    start();
    writeByte(type == TEMP? MEASURE_TEMP : MEASURE_HUMI);
    for (uint8_t i = 0; i < 250; ++i) {
        if (!digiRead()) {
            meas[type] = readByte(1) << 8;
            meas[type] |= readByte(1);
            uint8_t flipped = 0;
            for (uint8_t j = 0x80; j != 0; j >>= 1) {
                flipped >>= 1;
            }
            if (readByte(0) != flipped)
                break;
            return 0;
        }
        if (delayFun)
            delayFun();
        else
            delay(1);
    }
    connReset();
    return 1;
}

void SHT21::calculate(float& rh_true, float& t_C) const {
  
	t_C = (175.72 / 16384.0) * meas[TEMP] - 46.85;  //T= -46.85 + 175.72 * ST/2^16
	rh_true = -6.0 + 125.0 / 4096.0 * meas[HUMI];   // RH= -6 + 125 * SRH/2^16
	
    if (rh_true > 99) rh_true = 100;
    if (rh_true < 0.1) rh_true = 0.1;
} 
