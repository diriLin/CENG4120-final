#pragma once

#include "global.h"

class Netlist {
private:
    void read(std::string netlist_file);
public:
    Netlist(std::string netlist_file) {read(netlist_file);}
    void write(std::string output_file);
};