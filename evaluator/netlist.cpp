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

    int net_num = 0;

    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);
    iss >> net_num;

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

    assert_t(nets.size() == net_num);

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
        file << std::endl;
    }
    file.close();
    log() << "Finish writing." << std::endl;
}



void Netlist::read_result(std::string result_file) {
    std::ifstream file(result_file);
    if (!file.is_open()) {
        log(LOG_ERROR) << "Fail to open result file " << result_file << std::endl;
        assert_t(false);
    }
    log(LOG_INFO) << "Result file: " << result_file << std::endl;

    std::string line;
    int net_id;
    std::string net_name;
    int parent_id, child_id;
    while (std::getline(file, line)) {
        if (is_blank_line(line)) continue;

        std::istringstream iss(line);
        iss >> net_id >> net_name;
        if (net_id < nets.size() && nets[net_id].get_name() == net_name) {
            // start checking net
            Net& net = nets[net_id];
            while (std::getline(file, line)) {
                // blank line. finish reading all pips of this net
                if (is_blank_line(line)) break;

                std::istringstream iss(line);
                iss >> parent_id >> child_id;
                if (parent_id >= device.num_nodes) {
                    log(LOG_ERROR) << "No such node: " << parent_id << std::endl;
                    continue;
                }
                if (child_id >= device.num_nodes) {
                    log(LOG_ERROR) << "No such node: " << child_id << std::endl;
                    continue;
                }
                Node* parent = &device.nodes[parent_id];
                Node* child = &device.nodes[child_id];
                PIP pip(parent, child);
                net.add_pip(pip);
            }
        }
    }
}

int Netlist::check_congestion_from_pips(bool reset_used_by_net_id) {
    if (reset_used_by_net_id) {
        for (Node& node: device.nodes) {
            node.set_used_by_net_id(-1);
        }
    }

    auto check_node = [&](Node* node, Net& net) {
        if (node == nullptr) return;
        int used_by_net_id = node->get_used_by_net_id();
        if (used_by_net_id == -1) {
            node->set_used_by_net_id(net.get_id());
        } else if (used_by_net_id != net.get_id()) {
            congested_node_ids.insert(node->get_id());
        }
    };

    for (Net& net: nets) {
        auto& pips = net.get_pips();
        for (auto iter = pips.begin(); iter != pips.end(); iter++) {
            check_node(iter->parent, net);
            check_node(iter->child, net);
        }
    }

    return congested_node_ids.size();
}

int Netlist::check_successfully_routed_nets_from_pips() {
    int num_successfully_routed_nets = 0;
    for (Net& net: nets) {
        // use prev pointer may result in other problem.
        std::unordered_map<Node*, Node*> child_to_parent;
        auto& pips = net.get_pips();
        for (auto iter = pips.begin(); iter != pips.end(); iter++) {
            assert_t(iter->parent != nullptr);
            assert_t(iter->child != nullptr);
            bool find_child = false;
            for (Node* child: iter->parent->get_children()) {
                if (child == iter->child) {
                    find_child = true;
                    break;
                }
            }
            if (!find_child) continue;
            child_to_parent[iter->child] = iter->parent;
        }

        // check reachability of all sinks
        bool is_successfully_routed_net = true;
        Node* source_node = &device.nodes[net.get_source_node_id()];

        if (!quiet && net.get_sink_node_ids().size() == 0) {
            log() << "net " << net.get_id() << ": no sinks." << std::endl;
        }

        for (int sink_node_id: net.get_sink_node_ids()) {
            bool is_successfully_routed_sink = false;
            std::vector<int> path;
            Node* node = &device.nodes[sink_node_id];
            int watch_dog = 100000;
            while (watch_dog > 0) {
                path.emplace_back(node->get_id());

                if (child_to_parent.find(node) == child_to_parent.end()) {
                    // fail to find source node
                    if (!quiet) {
                        log(LOG_ERROR) << "net " << net.get_id() << ": fail to find source node." << std::endl;
                        auto& ofs = log(LOG_ERROR) << "path starting from sink: ";
                        for (int node_id: path) {
                            ofs << node_id << " ";
                        }
                        ofs << std::endl;
                    }
                    
                    break;
                }
                Node* parent = child_to_parent.at(node);
                if (parent == source_node) {
                    // find source node
                    is_successfully_routed_sink = true;
                    break;
                }
                node = parent;
                watch_dog--;
            }
            if (is_successfully_routed_sink == false) {
                if (!quiet) {
                    log(LOG_ERROR) << "net " << net.get_id() << ": fail to route sink node " << sink_node_id << "." << std::endl;
                    auto& ofs = log(LOG_ERROR) << "path starting from sink: ";
                    for (int node_id: path) {
                        ofs << node_id << " ";
                    }
                    ofs << std::endl;
                }
                
                is_successfully_routed_net = false;
                break;
            }
        }

        if (is_successfully_routed_net) {
            num_successfully_routed_nets++;
        }
    }

    return num_successfully_routed_nets;
}

int Netlist::check_total_wirelength_from_pips() {
    int total_wirelength = 0;
    std::unordered_set<int> used_node_ids;
    for (Net& net: nets) {
        auto& pips = net.get_pips();
        for (auto iter = pips.begin(); iter != pips.end(); iter++) {
            assert_t(iter->parent != nullptr);
            assert_t(iter->child != nullptr);
            used_node_ids.insert(iter->parent->get_id());
            used_node_ids.insert(iter->child->get_id());
        }
    }

    for (auto iter = used_node_ids.begin(); iter != used_node_ids.end(); iter++) {
        total_wirelength += device.nodes[*iter].get_length();
    }
    return total_wirelength;
}

void Netlist::evaluate_pips() {
    int num_congested_nodes = check_congestion_from_pips(true);
    int num_successfully_routed_nets = check_successfully_routed_nets_from_pips();
    int total_wirelength = check_total_wirelength_from_pips();
    log() << "# congested nodes: " << num_congested_nodes << std::endl;
    log() << "# successfully routed nets: " << num_successfully_routed_nets << "/" << nets.size() << std::endl;
    log() << "total wirelength: " << total_wirelength << std::endl;
}

void Netlist::evaluate(std::string result_file) {
    for (Net& net: nets) {
        net.clear_pips();
    }

    read_result(result_file);
    evaluate_pips();
}