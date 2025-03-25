#pragma once

#include "global.h"

class Device {
private:
    void read(std::string device_file);
public:
    Device(std::string device_file) {read(device_file);}
};