#pragma once

#include "global.h"

enum class IntentCode {
    NODE_LOCAL,
    NODE_PINFEED,
    NODE_VLONG,
    NODE_HQUAD,
    INTENT_DEFAULT,
    NODE_HLONG,
    NODE_PINBOUNCE,
    NODE_SINGLE,
    NODE_VQUAD,
    NODE_INT_INTERFACE,
    NODE_DOUBLE,
    NODE_CLE_OUTPUT,
};

enum class NodeType {
    PINBOUNCE,
    WIRE,
    PINFEED_I,
    PINFEED_O
};

class Node {
private:
    // hardware property
    int id;
    IntentCode intent_code;
    int length;
    int begin_x;
    int begin_y;
    int end_x;
    int end_y;
    // std::vector<Node*> children;
    NodeType node_type;
    int used_by_net_id = -1;

    // routing
    int occupancy = 0;
    double base_cost;
    double total_path_cost;
    double upstream_cost;
    double present_congestion_cost = 1;
    double historical_congestion_cost = 1;
    int last_visited_stamp = -1;
    int target_stamp = -1;
    bool accessible = true;
    Node* prev = nullptr;

    double compute_base_cost();
    NodeType compute_node_type();
    
public:
    Node() {}
    Node(int id_, IntentCode intent_code_, int length_, int begin_x_, int begin_y_, int end_x_, int end_y_): 
        id(id_), intent_code(intent_code_), length(length_), begin_x(begin_x_), begin_y(begin_y_), end_x(end_x_), end_y(end_y_) {
        base_cost = compute_base_cost();
        node_type = compute_node_type();
    }

    void reset(int new_id, IntentCode new_intent_code, int new_length, int new_begin_x, int new_begin_y, int new_end_x, int new_end_y) {
        id = new_id;
        intent_code = new_intent_code;
        length = new_length;
        begin_x = new_begin_x;
        begin_y = new_begin_y;
        end_x = new_end_x;
        end_y = new_end_y;

        base_cost = compute_base_cost();
        node_type = compute_node_type();
    }

    int get_id() {return id;}
    int get_length() {return length;}
    int get_begin_x() {return begin_x;}
    int get_begin_y() {return begin_y;}
    int get_end_x() {return end_x;}
    int get_end_y() {return end_y;}
    int get_occupancy() {return occupancy;}
    double get_base_cost() {return base_cost;}
    double get_total_path_cost() {return total_path_cost;}
    double get_upstream_cost() {return upstream_cost;}
    double get_present_congestion_cost() {return present_congestion_cost;}
    double get_historical_congestion_cost() {return historical_congestion_cost;}
    // std::vector<Node*>& get_children() {return children;}
    Node* get_prev() {return prev;}
    NodeType get_node_type() {return node_type;}
    int get_used_by_net_id() {return used_by_net_id;}

    bool is_visited(int connection_stamp) {return last_visited_stamp == connection_stamp;}
    bool is_target(int connection_stamp) {return target_stamp == connection_stamp;}
    bool is_congested() {return occupancy > 1;}
    bool is_accessible() {return accessible;}

    void set_base_cost(double base_cost_) {base_cost = base_cost_;}
    void set_total_path_cost(double cost_) {total_path_cost = cost_;}
    void set_upstream_cost(double upstream_cost_) {upstream_cost = upstream_cost_;}
    void set_present_congestion_cost(double present_congestion_cost_) {present_congestion_cost = present_congestion_cost_;}
    void set_historical_congestion_cost(double historical_congestion_cost_) {historical_congestion_cost = historical_congestion_cost_;}
    void set_visited(int connection_stamp) {last_visited_stamp = connection_stamp;}
    void set_target(int connection_stamp) {target_stamp = connection_stamp;}
    void increase_occupancy() {occupancy++;}
    void decrease_occupancy() {occupancy--;}
    // void add_child(Node* child) {children.emplace_back(child);}
    void update_present_congestion_cost(double pres_factor);
    void write_routing_info(Node* prev, double total_path_cost, double upstream_cost, int visited_stamp, int target_stamp);
    void set_prev(Node* prev_) {prev = prev_;}
    void set_accessible(bool accessible_) {accessible = accessible_;}
    void set_node_type(NodeType node_type_) {node_type = node_type_;}
    void set_used_by_net_id(int used_by_net_id_) {used_by_net_id = used_by_net_id_;}
};
class Device {
private:
    void read(std::string device_file);
public:
    int num_nodes;
    int num_threads = 8;
    std::vector<Node> nodes;
    std::vector<std::vector<Node*>> children;

    std::unordered_map<std::string, IntentCode> str_to_ic = {
        {"NODE_LOCAL", IntentCode::NODE_LOCAL},
        {"NODE_PINFEED", IntentCode::NODE_PINFEED},
        {"NODE_VLONG", IntentCode::NODE_VLONG},
        {"NODE_HQUAD", IntentCode::NODE_HQUAD},
        {"INTENT_DEFAULT", IntentCode::INTENT_DEFAULT},
        {"NODE_HLONG", IntentCode::NODE_HLONG},
        {"NODE_PINBOUNCE", IntentCode::NODE_PINBOUNCE},
        {"NODE_SINGLE", IntentCode::NODE_SINGLE},
        {"NODE_VQUAD", IntentCode::NODE_VQUAD},
        {"NODE_INT_INTERFACE", IntentCode::NODE_INT_INTERFACE},
        {"NODE_DOUBLE", IntentCode::NODE_DOUBLE},
        {"NODE_CLE_OUTPUT", IntentCode::NODE_CLE_OUTPUT}
    };

    Device(std::string device_file) {read(device_file);}
};

