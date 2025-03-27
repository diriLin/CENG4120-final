#include "device.h"

// --------------Node---------------
void Node::update_present_congestion_cost(double pres_factor) {
    if (occupancy < 1) {
        set_present_congestion_cost(1);
    } else {
        set_present_congestion_cost(1 + occupancy * pres_factor);
    }
}

void Node::write_routing_info(Node* prev_, double cost_, double upstream_cost_, int last_visted_stamp_, int target_stamp_) {
    prev = prev_;
    total_path_cost = cost_;
    upstream_cost = upstream_cost_;
    last_visited_stamp = last_visted_stamp_;
    target_stamp = target_stamp_;
}

double Node::compute_base_cost() {
    double bc = 0.4;
    switch (intent_code){
        case IntentCode::NODE_CLE_OUTPUT:
        case IntentCode::NODE_LOCAL:
        case IntentCode::INTENT_DEFAULT:
            break;
        case IntentCode::NODE_SINGLE:
            assert_t(length <= 2);
            if (length == 2) bc *= length;
            break;
        case IntentCode::NODE_DOUBLE:
            if (end_x != begin_x) {
                assert_t(length <= 2);
                if (length == 2) bc *= length;
            } else {
                assert_t(length <= 3);
            }
            break;
        case IntentCode::NODE_HQUAD:
            if (length == 0) {
                accessible = false;
                log(LOG_DEBUG) << "Node " << id << " is set inaccessible." << std::endl;
            } else {
                bc = 0.35 * length;// HLONGs have length 6 and 7
            }
            break;
        case IntentCode::NODE_VQUAD:
            if (length != 0) bc = 0.15 * length;
            break;
        case IntentCode::NODE_HLONG:
            if (length == 0) {
                accessible = false;
                log(LOG_DEBUG) << "Node " << id << " is set inaccessible." << std::endl;
            } else {
                bc = 0.15 * length;// HLONGs have length 6 and 7
            }
            break;
        case IntentCode::NODE_VLONG:
            bc = 0.7 * length;
            break;
        case IntentCode::NODE_PINFEED:
        case IntentCode::NODE_PINBOUNCE:
        case IntentCode::NODE_INT_INTERFACE:
            break;
        
        default:
            break;
    }
    return bc;
}

NodeType Node::compute_node_type() {
    return intent_code == IntentCode::NODE_PINBOUNCE ? NodeType::PINBOUNCE : NodeType::WIRE;
}

// --------------Device---------------
void Device::read(std::string device_file) {
    std::ifstream file(device_file);
    if (!file.is_open()) {
        log(LOG_ERROR) << "Fail to open device file " << device_file << std::endl;
        assert_t(false);
    }
    log(LOG_INFO) << "Device file: " << device_file << std::endl;
    
    std::string line;

    // get #nodes
    std::getline(file, line);
    num_nodes = std::stoi(line);
    log() << "#nodes: " << num_nodes << std::endl;
    nodes.reserve(num_nodes);

    // create nodes
    int id, length, begin_x, begin_y, end_x, end_y;
    int parent_id, child_id;
    std::string intent_code;
    for (int i = 0; i < num_nodes; i++) {
        std::getline(file, line);
        std::istringstream iss(line);
        iss >> id >> intent_code >> length >> begin_x >> begin_y >> end_x >> end_y;
        nodes.emplace_back(id, str_to_ic[intent_code], length, begin_x, begin_y, end_x, end_y);
    }
    std::getline(file, line);
    log() << "Finish reading nodes." << std::endl;

    // add children
    for (int i = 0; i < num_nodes; i++) {
        std::getline(file, line);
        std::istringstream iss(line);
        iss >> parent_id;
        while (iss >> child_id) {
            nodes[parent_id].add_child(&nodes[child_id]);
        }
    }
    log() << "Finish reading edges." << std::endl;

    file.close();
}