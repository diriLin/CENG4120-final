#pragma once

#include "global.h"
#include "device.h"

class Connection {
private:
    // info
    int id;
    int net_id;
    Node* source_node = nullptr;
    Node* sink_node = nullptr;
    
    // geo
    int xmin = 1000000;
    int xmax = -100000;
    int ymin = 1000000;
    int ymax = -100000;

    // routing
    bool routed = false;
    std::vector<Node*> path;
    
public: 
    Connection() {}
    Connection(int id_, int net_id_, Node* source_node_, Node* sink_node_);

    int get_id() {return id;}
    int get_net_id() {return net_id;}
    Node* get_source_node() {return source_node;}
    Node* get_sink_node() {return sink_node;}
    int get_xmin() {return xmin;}
    int get_xmax() {return xmax;}
    int get_ymin() {return ymin;}
    int get_ymax() {return ymax;}
    int get_hpwl() {return xmax - xmin + ymax - ymin;}
    std::vector<Node*>& get_path() {return path;}

    bool is_routed() {return routed;}
    bool is_congested();
    bool is_accessible_node(Node* node);
    
    void set_xmin(int xmin_) {xmin = xmin_;}
    void set_xmax(int xmax_) {xmax = xmax_;}
    void set_ymin(int ymin_) {ymin = ymin_;}
    void set_ymax(int ymax_) {ymax = ymax_;}
    void set_bbox(int xmin_, int xmax_, int ymin_, int ymax_) {xmin = xmin_; xmax = xmax_; ymin = ymin_; ymax = ymax_;}
    void set_routed(bool routed_) {routed = routed_;}

    std::string to_string();
    void reset_route() {routed = false; path.clear();}
    void add_node(Node* node) {path.emplace_back(node);}
};

class PIP {
public:
    Node* parent = nullptr;
    Node* child = nullptr;

    PIP() {}
    PIP(Node* parent_, Node* child_): parent(parent_), child(child_) {}
    std::string to_string() {std::stringstream ss; ss << parent->get_id() << " " << child->get_id(); return ss.str();}
};

struct PIP_hash {
    size_t operator()(const PIP& pip) const {
        return std::hash<int>()(pip.parent->get_id()) ^ (std::hash<int>()(pip.child->get_id()) << 1);
    }
};

struct PIP_equal {
    bool operator()(const PIP& a, const PIP& b) const {
        return a.parent->get_id() == b.parent->get_id() && a.child->get_id() == b.child->get_id();
    }
};

class Net {
private:
    // info
    int id;
    std::string name;
    int source_node_id;
    std::vector<int> sink_node_ids;
    std::vector<int> connection_ids;
    std::unordered_set<PIP, PIP_hash, PIP_equal> pips;
    // node id -> # connections using node
    std::unordered_map<Node*, int> user_connection_cnts;

    // geo
    int xmin = 1000000;
    int xmax = -100000;
    int ymin = 1000000;
    int ymax = -100000;

    double xcenter;
    double ycenter;

public:
    Net() {};
    Net(int id_, std::string name_, int source_node_id_, std::vector<int>& sink_node_ids_):
        id(id_), name(name_), source_node_id(source_node_id_), sink_node_ids(sink_node_ids_) {user_connection_cnts.clear();}

    int get_id() {return id;}
    std::string get_name() {return name;}
    int get_source_node_id() {return source_node_id;}
    std::vector<int>& get_sink_node_ids() {return sink_node_ids;}
    std::vector<int>& get_connection_ids() {return connection_ids;}
    int get_num_connection() {return connection_ids.size();}
    std::unordered_set<PIP, PIP_hash, PIP_equal>& get_pips() {return pips;}
    int get_xmin() {return xmin;}
    int get_xmax() {return xmax;}
    int get_ymin() {return ymin;}
    int get_ymax() {return ymax;}
    double get_xcenter() {return xcenter;}
    double get_ycenter() {return ycenter;}
    int get_hpwl() {return 2 * (xmax - xmin + 1 + ymax - ymin + 1);}

    void set_connection_ids(std::vector<int>& connection_ids_) {connection_ids = connection_ids_;}
    void set_xmin(int xmin_) {xmin = xmin_;}
    void set_xmax(int xmax_) {xmax = xmax_;}
    void set_ymin(int ymin_) {ymin = ymin_;}
    void set_ymax(int ymax_) {ymax = ymax_;}
    void set_xcenter(double xcenter_) {xcenter = xcenter_;}
    void set_ycenter(double ycenter_) {ycenter = ycenter_;}
    void add_connection_id(int connection_id) {connection_ids.emplace_back(connection_id);}
    void add_pip(PIP pip) {pips.insert(pip);}

    // Return: # connections using node (in this net)
    int count_user_connections(Node* node);
    // Increase user connection count of this node.
    // If previous user connection count == 0, increase the occupancy of this node.
    void increase_user(Node* node);
    // Decrease user connection count of this node.
    // If previous user connection count == 1, decrease the occupancy of this node.
    void decrease_user(Node* node);
    std::string to_string();
};

class Netlist {
private:
    Device& device;
    void read(std::string netlist_file);
    void set_connections();
    void compute_bbox_and_center_of_nets();
public:
    std::vector<Net> nets;
    std::vector<Connection> connections;
    Netlist(Device& device_, std::string netlist_file): device(device_) {read(netlist_file);}
    void write(std::string output_file);
};