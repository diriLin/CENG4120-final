#pragma once

#include "global.h"
#include "device.h"
#include "netlist.h"

class Router {
private:
    Device& device;
    Netlist& netlist;

public:
    Router(Device& device_, Netlist& netlist_): device(device_), netlist(netlist_) {}
    void route();
};