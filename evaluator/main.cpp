#include "global.h"
#include "device.h"
#include "netlist.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        log(LOG_ERROR) << "Usage: ./eval <device> <netlist> <result>" << std::endl;
        return 1;
    }

    std::string device_file = argv[1];
    std::string netlist_file = argv[2];
    std::string output_file = argv[3];

    Device device(device_file);
    Netlist netlist(device, netlist_file);

    // check
    netlist.evaluate(output_file);

    log() << "Exit." << std::endl;
    return 0;
}