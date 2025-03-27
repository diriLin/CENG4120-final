#include "netlist.h"

// ------------------------ Connection --------------------------------

Connection::Connection(int id_, int net_id_, Node* source_node_, Node* sink_node_): id(id_), net_id(net_id_), source_node(source_node_), sink_node(sink_node_) {
    xmin = std::min(xmin, source_node_->get_begin_x());
    xmin = std::min(xmin, source_node_->get_end_x());
    xmin = std::min(xmin, sink_node_->get_begin_x());
    xmin = std::min(xmin, sink_node_->get_end_x());
    xmin -= 3;
    xmax = std::max(xmax, source_node_->get_begin_x());
    xmax = std::max(xmax, source_node_->get_end_x());
    xmax = std::max(xmax, sink_node_->get_begin_x());
    xmax = std::max(xmax, sink_node_->get_end_x());
    xmax += 3;
    ymin = std::min(ymin, source_node_->get_begin_y());
    ymin = std::min(ymin, source_node_->get_end_y());
    ymin = std::min(ymin, sink_node_->get_begin_y());
    ymin = std::min(ymin, sink_node_->get_end_y());
    ymin -= 15;
    ymax = std::max(ymax, source_node_->get_begin_y());
    ymax = std::max(ymax, source_node_->get_end_y());
    ymax = std::max(ymax, sink_node_->get_begin_y());
    ymax = std::max(ymax, sink_node_->get_end_y());
    ymax += 15;
}

bool Connection::is_congested() {
    for (Node* node: path) {
        if (node->is_congested()) {
            return true;
        }
    }
    return false;
}

std::string Connection::to_string() {
    std::stringstream ss;
    ss << "conn id: " << id << " net id: " << net_id << " source node: " << source_node->get_id() << " sink node: " << sink_node->get_id() << " bbox: [" << xmin << ", " << ymin << "] -> [" << xmax << ", " << ymax << "]";
    return ss.str();
}

bool Connection::is_accessible_node(Node* node) {
    int end_x = node->get_end_x();
    int end_y = node->get_end_y();
    return end_x > xmin && end_x < xmax && end_y > ymin && end_y < ymax;
}

// ------------------------ Net --------------------------------

int Net::count_user_connections(Node* node) {
    return user_connection_cnts.find(node) == user_connection_cnts.end() ? 0 :user_connection_cnts.at(node);
}

void Net::increase_user(Node* node) {
    if (user_connection_cnts.find(node) == user_connection_cnts.end()) {
        user_connection_cnts[node] = 1;
        node->increase_occupancy();
    } else {
        user_connection_cnts[node] ++;
    }
}

void Net::decrease_user(Node* node) {
    if (user_connection_cnts.find(node) == user_connection_cnts.end()) {
        log(LOG_ERROR) << "No connections from net " << id << " using node " << node->get_id() << " but try to decrease!" << std::endl;
        exit(1);
    }
    user_connection_cnts[node] --;

    if (user_connection_cnts.at(node) == 0) {
        user_connection_cnts.erase(node);
        node->decrease_occupancy();
    }
}

std::string Net::to_string() {
    std::stringstream ss;
    ss << id << " " << name << " " << source_node_id;
    for (int sink_node_id: sink_node_ids) {
        ss << " " << sink_node_id;
    }
    return ss.str();
}

// ------------------------ Netlist --------------------------------

void Netlist::read(std::string netlist_file) {
    std::ifstream file(netlist_file);
    if (!file.is_open()) {
        log(LOG_ERROR) << "Fail to open netlist file " << netlist_file << std::endl;
        assert_t(false);
    }
    log(LOG_INFO) << "Netlist file: " << netlist_file << std::endl;

    std::string line;

    int id, source_node_id, sink_node_id;
    std::string name;
    std::vector<int> sink_node_ids;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> id >> name >> source_node_id;
        sink_node_ids.clear();
        while (iss >> sink_node_id) {
            sink_node_ids.emplace_back(sink_node_id);
        }
        nets.emplace_back(id, name, source_node_id, sink_node_ids);
    }

    file.close();
    set_connections();
    log() << "Finish reading netlist." << std::endl;
}

void Netlist::set_connections() {
    for (Net& net: nets) {
        int net_id = net.get_id();
        Node* source_node = &device.nodes[net.get_source_node_id()];
        source_node->set_node_type(NodeType::PINFEED_O);
        connections.reserve(net.get_sink_node_ids().size());
        for (int sink_node_id: net.get_sink_node_ids()) {
            int connection_id = connections.size();
            Node* sink_node = &device.nodes[sink_node_id];
            sink_node->set_node_type(NodeType::PINFEED_I);
            connections.emplace_back(connection_id, net_id, source_node, sink_node);
            net.add_connection_id(connection_id);
        }
    }
}

void Netlist::compute_bbox_and_center_of_nets() {
    for (Net& net: nets) {
        double x_sum = 0;
        double y_sum = 0;
        int cnt = 0;
        bool source_node_added = false;
        for (int connection_id: net.get_connection_ids()) {
            Connection& connection = connections[connection_id];
            if (!source_node_added) {
                int x = connection.get_source_node()->get_end_x();
                int y = connection.get_source_node()->get_end_y();
                net.set_xmin(std::min(net.get_xmin(), x));
                net.set_xmax(std::max(net.get_xmax(), x));
                net.set_ymin(std::min(net.get_ymin(), y));
                net.set_ymax(std::max(net.get_ymax(), y));
                x_sum += x;
                y_sum += y;
                cnt ++;
                source_node_added = true;
            }
            int x = connection.get_sink_node()->get_end_x();
            int y = connection.get_sink_node()->get_end_y();
            net.set_xmin(std::min(net.get_xmin(), x));
            net.set_xmax(std::max(net.get_xmax(), x));
            net.set_ymin(std::min(net.get_ymin(), y));
            net.set_ymax(std::max(net.get_ymax(), y));
            x_sum += x;
            y_sum += y;
            cnt ++;
        }

        net.set_xcenter(double(x_sum) / cnt);
        net.set_ycenter(double(y_sum) / cnt);
    }
}

void Netlist::write(std::string output_file) {
    std::ofstream file(output_file);
    if (!file.is_open()) {
        log(LOG_ERROR) << "Fail to open output file " << output_file << std::endl;
        assert_t(false);
    }
    log(LOG_INFO) << "Output file: " << output_file << std::endl;
    for (Net& net: nets) {
        file << net.get_id() << " " << net.get_name() << std::endl;
        auto& pips = net.get_pips();
        for (auto iter = pips.begin(); iter != pips.end(); iter++) {
            file << iter->parent->get_id() << " " << iter->child->get_id() << std::endl;
        }
    }
    file << std::endl;
    file.close();
}
