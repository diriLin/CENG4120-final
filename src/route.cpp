#include "route.h"

void Router::route() {
    sort_connections();
    update_sink_node_occupancy();
    print_header();
    iterative_route();
    save_pips();
}

void Router::sort_connections() {
    sorted_connection_ids.clear();
    for (int i = 0; i < connections.size(); i++) {
        sorted_connection_ids.emplace_back(i);
    }

    auto compare = [this](const int& i, const int& j) {
        // 1st priority: descending net fanout
        int net_fanout_i = nets[connections[i].get_net_id()].get_num_connection();
        int net_fanout_j = nets[connections[j].get_net_id()].get_num_connection();
        if (net_fanout_i != net_fanout_j)
            return net_fanout_i > net_fanout_j;

        // 2nd priority: ascending HPWL
        return connections[i].get_hpwl() < connections[j].get_hpwl();
    };

    std::sort(sorted_connection_ids.begin(), sorted_connection_ids.end(), compare);
}

void Router::update_sink_node_occupancy() {
    for (Connection& connection: connections) {
        Node* sink_node = connection.get_sink_node();
        Net& net = nets[connection.get_net_id()];
        net.increase_user(sink_node);
        sink_node->update_present_congestion_cost(present_congestion_factor);
        if (sink_node->get_occupancy() > 1 || net.count_user_connections(sink_node) > 1) {
            log(LOG_ERROR) << "Node " << sink_node << " is used by multiple connections." << std::endl;
            assert_t(false);
        }
    }
}

void Router::print_header() {
    log() << "---------------------------------------------------------------------------------------------------------------------------" << std::endl;
	log() << std::setw(10) << "Iteration" << std::setw(15) << "PFactor" << std::setw(10) << "HFactor" << std::setw(20) << "RoutedConnections" << std::setw(15) << "CongestedNodes" << std::setw(8) << "Times" << std::endl;
}

void Router::print_route_stat() {
    log()<< std::setw(10) << iter << std::setw(15) << present_congestion_factor << std::setw(10) << historical_congestion_factor << std::setw(20) << num_routed_connection << std::setw(15) << num_congested_nodes << std::fixed << std::setw(8) << std::setprecision(2) << timer.elapsed() << std::endl;
}

void Router::rip_up(Connection& connection) {
    std::vector<Node*>& path = connection.get_path();
    if (path.empty()) {
        assert_t(!connection.is_routed());
        path.emplace_back(connection.get_sink_node());
    }
    Net& net = nets[connection.get_net_id()];
    for (Node* node: path) {
        net.decrease_user(node);
        node->update_present_congestion_cost(present_congestion_factor);
    }
    connection.reset_route();
}

void Router::iterative_route() {
    for (iter = 1; iter <= max_iter; iter ++) {
        timer.start();
        connection_stamp_base += connections.size();
        num_routed_connection = 0;
        num_failed_connection = 0;
        for (int connection_id: sorted_connection_ids) {
            Connection& connection = connections[connection_id];
            if (should_route(connection)) {
                rip_up(connection);
                bool success = route_connection(connection);
                if (!success) {
                    num_failed_connection ++;
                    log() << "Routing failure: " << connection.to_string();
                }
            }
        }

        if (iter == 1) {
            decide_congested_design();
        }
        update_cost_factors();
        print_route_stat();

        if (num_congested_nodes == 0 && num_failed_connection == 0)
			break;
    }
    log() << "---------------------------------------------------------------------------------------------------------------------------" << std::endl;
    log() << "Finish routing." << std::endl;
}

bool Router::route_connection(Connection& connection) {
    num_routed_connection ++;
    Net& net = nets[connection.get_net_id()];
    Node* source_node = connection.get_source_node();
    Node* sink_node = connection.get_sink_node();
    // connection_stamp = iter * # connections + connection id
    // a unique stamp for visited and target checking
    int connection_stamp = connection_stamp_base + connection.get_id();
    auto node_compare = [&] (Node* lhs, Node* rhs) {return lhs->get_total_path_cost() > rhs->get_total_path_cost();};
    std::priority_queue<Node*, std::vector<Node*>, decltype(node_compare)> node_queue(node_compare);
    auto push = [&](Node* node, Node* prev, double total_path_cost, double upstream_cost, int target_stamp) {
		node->write_routing_info(prev, total_path_cost, upstream_cost, connection_stamp, target_stamp);
		node_queue.push(node);
	};

    push(source_node, nullptr, 0, 0, -1);
    sink_node->write_routing_info(nullptr, 0, 0, -1, connection_stamp);
    Node* target_node = nullptr;

    while (!node_queue.empty()) {
        Node* node = node_queue.top(); node_queue.pop();
        assert_t(node != nullptr);
        double upstream_cost = node->get_upstream_cost();
        for (Node* child: node->get_children()) {
            if (child->is_visited(connection_stamp)) {
                continue;
            }
            bool is_target = child->is_target(connection_stamp);
            if (is_target) {
                target_node = child;
                child->set_prev(node);
                break;
            }
            if (!connection.is_accessible_node(node)) {
                continue;
            }
            switch (child->get_node_type()) {
                case NodeType::WIRE:
                case NodeType::PINBOUNCE:
                case NodeType::PINFEED_O:
                    break;
                case NodeType::PINFEED_I:
                    if (!node->is_target(connection_stamp)) continue;
                    break;
                default:
                    log(LOG_ERROR) << "Unknown node type of node" << child->get_id() << std::endl;
            }

            int user_connection_cnt = net.count_user_connections(child);
            double sharing_factor = 1 + sharing_weight * user_connection_cnt;
            double cost_of_child = get_cost(child, connection, user_connection_cnt, sharing_factor, is_target);
            assert_t(cost_of_child >= 0);
            double upstream_cost_of_child = upstream_cost + node_cost_weight * cost_of_child + node_WL_weight * node->get_length() / sharing_factor;
            int delta_x = std::abs(child->get_end_x() - sink_node->get_begin_x());
            int delta_y = std::abs(child->get_end_y() - sink_node->get_begin_y());
            double total_path_cost_of_child = upstream_cost_of_child + est_WL_weight * (delta_x + delta_y) * 1.0 / sharing_factor;
            push(child, node, total_path_cost_of_child, upstream_cost_of_child, -1);
        }
        if (target_node != nullptr) {
            break;
        }
    }
    
    if (target_node == nullptr) {
        assert_t(node_queue.empty());
        return false;
    }

    bool routed = save_routing(connection, target_node);
    if (routed) {
        connection.set_routed(true);
        update_users_and_present_congestion_cost(connection);
    } else {
        connection.reset_route();
    }

    return routed;
}

double Router::get_cost(Node* node, Connection& connection, int user_connection_cnt, double sharing_factor, bool is_target) {
    double present_congestion_cost;
    if (user_connection_cnt != 0) {
        present_congestion_cost = 1 + (node->get_occupancy() - 1) * present_congestion_factor;
    } else {
        present_congestion_cost = node->get_present_congestion_cost();
    }

    double bias_cost = 0;
    int connection_stamp = connection.get_id() + connection_stamp_base;
    if (!node->is_target(connection_stamp)) {
        Net& net = nets[connection.get_net_id()];
        double dist_to_center = std::abs(node->get_end_x() - net.get_xcenter()) + std::abs(node->get_end_y() - net.get_ycenter());
        bias_cost = node->get_base_cost() / net.get_num_connection() * dist_to_center / net.get_hpwl();
    }

    double cost = node->get_base_cost() * node->get_historical_congestion_cost() * present_congestion_cost / sharing_factor + bias_cost;
    if (cost < 0) {
        log(LOG_ERROR) << "Node " << node->get_id() << " cost: " << cost << " user_connection_cnt: " << user_connection_cnt << " base cost: " << node->get_base_cost() << " h-cost: " << node->get_historical_congestion_cost() << " p-cost: " << present_congestion_cost << " sharing factor: " << sharing_factor << std::endl;
    }
    return cost;
}

bool Router::save_routing(Connection& connection, Node* target_node) {
    assert_t(target_node->get_id() == connection.get_sink_node()->get_id());
    int watch_dog = 100000;
    Node* node = target_node;
    do {
        if (node->get_prev() == nullptr) {
            assert_t(node->get_id() == connection.get_source_node()->get_id());
        }
        watch_dog --;
        connection.add_node(node);
        if (watch_dog == 0) {
            log(LOG_ERROR) << "Watch dog: " << connection.to_string() << std::endl;
            assert_t(false);
        }
        node = node->get_prev();
    } while (node != nullptr);

    assert_t(connection.get_path().size() > 1);
    return true;
}

void Router::update_users_and_present_congestion_cost(Connection& connection) {
    Net& net = nets[connection.get_net_id()];
    for (Node* node: connection.get_path()) {
        net.increase_user(node);
        node->update_present_congestion_cost(present_congestion_factor);
    }
}

void Router::update_cost_factors() {
	if (is_congested_design) {
		double r = 1.0 / (1 + exp((1 - iter) * 0.5));
		historical_congestion_factor = 2 * r;
		double r2 = 3.0 / (1 + exp((iter - 1)));
		present_congestion_multiplier = 1.1 * (1 + r2);
	}

	present_congestion_factor *= present_congestion_multiplier;
	present_congestion_factor = std::min(present_congestion_factor, max_present_congestion_factor);

	num_congested_nodes = 0;
	for (Node& node: device.nodes) {
        int overuse = node.get_occupancy() - 1;
        if (overuse == 0) {
            node.set_present_congestion_cost(1 + present_congestion_factor);
        } else if (overuse > 0) {
            num_congested_nodes++;
            node.set_present_congestion_cost(1 + (overuse + 1) * present_congestion_factor);
            node.set_historical_congestion_cost(node.get_historical_congestion_cost() + overuse * historical_congestion_factor);
        }
    }
}

void Router::decide_congested_design() {
    num_congested_nodes = 0;
    for (Node& node: device.nodes) {
        if (node.is_congested()) {
            num_congested_nodes ++;
        }	
    }
    /* determine the congested design based on the ratio of overused rnode number to the number of connections */ 
    congest_ratio = num_congested_nodes * 1.0 / connections.size();
    if (congest_ratio > 0.45) // 0.5 -> 0.45 for new cost function
        is_congested_design = true;
}

void Router::save_pips() {
    // check congestion
    std::unordered_set<int> congested_node_ids;
    for (Net& net: nets) {
        for (int connection_id: net.get_connection_ids()) {
            Connection& connection = connections[connection_id];
            for (Node* node: connection.get_path()) {
                int used_by_net_id = node->get_used_by_net_id();
                if (used_by_net_id == -1) {
                    node->set_used_by_net_id(net.get_id());
                } else if (used_by_net_id != net.get_id()) {
                    congested_node_ids.insert(node->get_id());
                }
            }
        }
    }
    log() << "# congested nodes: " << congested_node_ids.size() << std::endl;

    for (Net& net: nets) {
        for (int connection_id: net.get_connection_ids()) {
            Connection& connection = connections[connection_id];
            std::vector<Node*>& path = connection.get_path();
            for (int i = 0; i < path.size() - 1; i++) {
                Node* child = path[i];
                Node* parent = path[i+1];

                bool find_child = false;
                for (Node* c: parent->get_children()) {
                    if (c->get_id() == child->get_id()) {
                        find_child = true;
                        break;
                    }
                }
                if (!find_child)
                    log(LOG_ERROR) << "Wrong PIP " << parent->get_id() << " -> " << child->get_id() << std::endl;

                PIP pip(parent, child);
                net.add_pip(pip);
            }
        }
    }

}