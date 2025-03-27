#include "global.h"
#include "device.h"
#include "netlist.h"
#include "route.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        log(LOG_ERROR) << "Usage: ./route <device> <input netlist> <output result>" << std::endl;
        return 1;
    }

    std::string device_file = argv[1];
    std::string netlist_file = argv[2];
    std::string output_file = argv[3];

    Device device(device_file);
    Netlist netlist(device, netlist_file);
    Router router(device, netlist);
    router.route();
    netlist.write(output_file);

    log() << "Exit." << std::endl;
    return 0;
}