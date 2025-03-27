#pragma once

#include <algorithm>
#include <iomanip>
#include <cmath>

#include "global.h"
#include "device.h"
#include "netlist.h"

class Router {
private:
    Device& device;
    Netlist& netlist;

    std::vector<Net>& nets;
    std::vector<Connection>& connections;
    std::vector<int> sorted_connection_ids;

    // parameters
    int max_iter = 500;
    double present_congestion_multiplier = 2;
	double max_present_congestion_factor = 1000000;
    double present_congestion_factor = 0.5;
    double historical_congestion_factor = 1;
	double node_cost_weight = 1;
	double sharing_weight = 1;
	double node_WL_weight = 0.2;
	double est_WL_weight = 0.8;

    // global variables
    int iter = 0;
    int num_routed_connection = 0;
    int num_failed_connection = 0;
    utils::timer timer;
    int connection_stamp_base = 0;
    bool congest_ratio = -1;
    bool is_congested_design = false;
    int num_congested_nodes;

    void sort_connections();
    void update_sink_node_occupancy();
    void print_header();
    void print_route_stat();
    void iterative_route();
    bool should_route(Connection& connection) {return !connection.is_routed() || connection.is_congested();}
    void rip_up(Connection& connection);
    bool route_connection(Connection& connection);
    double get_cost(Node* node, Connection& connection, int user_connection_cnt, double sharing_factor, bool is_target);
    bool save_routing(Connection& connection, Node* target_node);
    void update_users_and_present_congestion_cost(Connection& connection);
    void update_cost_factors();
    void decide_congested_design();
    void save_pips();

public:
    Router(Device& device_, Netlist& netlist_): 
        device(device_), netlist(netlist_), nets(netlist_.nets), connections(netlist_.connections) {}
    void route();
    
};