/*
MIT License

Copyright (c) [2025] [azula1A89]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include "json.hpp"
#include "inja.hpp"
namespace mindmap{
using namespace std;

typedef enum class arrow_e{ normal,dual,none, }arrow_t;
typedef enum class object_e{ NODE, EDGE, CLUSTER }object_t;
typedef enum class object_color_e{ bd_color, bg_color, txt_color, edge_color}object_color_t;

class node_object{
public:
    int m_id;
    int m_seq;
    bool m_focused;
    bool m_locked;
    int m_indeg;
    int m_outdeg;
    int m_deg;
    string m_label;
    float m_label_font_size;
    float m_label_margin;
    ImVec2 m_label_sz;
    ImRect m_rect;
    ImU32 m_bd_col;
    ImU32 m_bg_col;
    ImU32 m_txt_col;
    string m_status;
    ImTextureID m_image;
    ImRect m_image_rect;
    object_t m_type;
public:
    node_object() = default;
    node_object(int id): m_id(id), m_seq(0), m_focused(false), m_locked(false), m_indeg(0), m_outdeg(0), m_deg(0), m_label("node"), m_label_font_size(12.0f), m_label_margin(0.0f), m_bd_col(ImU32(ImColor(ImGuiCol_Border))), m_bg_col(ImU32(ImColor(ImGuiCol_WindowBg))), m_txt_col(ImU32(ImColor(ImGuiCol_Text))), m_type(object_t::NODE){};
    bool operator==(const node_object& rhs){ return (m_id == rhs.m_id); };
    string& label(){ return m_label; };
    ImU32& bd_col(){ return m_bd_col; };
    ImU32& bg_col(){ return m_bg_col; };
    ImU32& txt_col(){ return m_txt_col; };
};

class edge_object{
public:
    int m_id;
    int m_seq;
    shared_ptr<node_object> m_source;
    shared_ptr<node_object> m_target;
    vector<ImVec2> m_bends;
    vector<ImVec2> m_polyline;
    bool m_focused;
    string m_label;
    float m_label_position;
    float m_ideal_length;
    float m_flow_position;
    float m_total_length;
    arrow_t m_arrow;
    ImVec2 m_arrow_ear1;
    ImVec2 m_arrow_ear2;
    ImVec2 m_arrow_ear3;
    ImVec2 m_arrow_ear4;
    float m_label_font_size;
    ImRect m_label_rect;
    ImU32 m_edge_col;
    ImU32 m_txt_col;
    object_t m_type;
public:
    edge_object() = default;
    edge_object(int id, shared_ptr<node_object> source, shared_ptr<node_object> target): m_id(id), m_source(source), m_target(target), m_seq(0), m_focused(false), m_label("edge"), m_label_font_size(12.0f), m_arrow(arrow_t::normal), m_label_position(0.5f), m_ideal_length(350.0f), m_flow_position(0.0f), m_total_length(0.0f), m_edge_col(ImU32(ImColor(ImGuiCol_Border))), m_txt_col(ImU32(ImColor(ImGuiCol_Text))), m_type(object_t::EDGE){};
    bool operator==(const edge_object& rhs){ return (m_id == rhs.m_id); };
    string& label(){ return m_label; };
    ImU32& edge_col(){ return m_edge_col; };
    ImU32& txt_col(){ return m_txt_col; };
};

class cluster_object{
public:
    int m_id;
    int m_seq;
    bool m_focused;
    float m_margin;
    float m_padding;
    string m_label;
    float m_label_font_size;
    ImRect m_rect;
    ImRect m_label_rect;
    ImU32 m_bd_col;
    ImU32 m_bg_col;
    ImU32 m_txt_col;
    vector<shared_ptr<node_object>> m_child_nodes;
    object_t m_type;
public:
    cluster_object() = default;
    cluster_object(int id): m_id(id), m_seq(0), m_focused(false), m_margin(0.0f), m_padding(0.0f), m_label("cluster"), m_label_font_size(12.0f), m_bd_col(ImU32(ImColor(ImGuiCol_Border))), m_bg_col(ImU32(ImColor(ImGuiCol_WindowBg))), m_txt_col(ImU32(ImColor(ImGuiCol_Text))), m_type(object_t::CLUSTER){};
    bool operator==(const cluster_object& rhs){ return (m_id == rhs.m_id); };
    bool operator!=(const cluster_object& rhs){ return (m_id != rhs.m_id); };
    string& label(){ return m_label; };
    ImU32& bd_col(){ return m_bd_col; };
    ImU32& bg_col(){ return m_bg_col; };
    ImU32& txt_col(){ return m_txt_col; };
};

using weak_node = weak_ptr<node_object>;
using weak_edge = weak_ptr<edge_object>;
using weak_cluster = weak_ptr<cluster_object>;
using node = shared_ptr<node_object>;
using edge = shared_ptr<edge_object>;
using cluster = shared_ptr<cluster_object>;
using node_pair = pair<int, node>;
using edge_pair = pair<int, edge>;
using cluster_pair = pair<int, cluster>;
using node_map = map<int, node>;
using edge_map = map<int, edge>;
using cluster_map = map<int, cluster>;

class Graph {
public:
    Graph(){ reset(); };
    
private:
    map<int, shared_ptr<node_object>> m_nodes;
    map<int, shared_ptr<edge_object>> m_edges;
    map<int, shared_ptr<cluster_object>> m_clusters;
    ImRect m_bbox;
    int m_node_max_index = 0;
    int m_edge_max_index = 0;
    int m_cluster_max_index = 0;
    int update_count = 0;
public:
    unordered_map<int, weak_node> m_weak_nodes;
    unordered_map<int, weak_edge> m_weak_edges;
    unordered_map<int, weak_cluster> m_weak_clusters;
public:
    int& node_max_index(){return m_node_max_index;};
    int& edge_max_index(){return m_edge_max_index;};
    int& cluster_max_index(){return m_cluster_max_index;};
    node_map& nodes(){return m_nodes;};
    edge_map& edges(){return m_edges;};
    cluster_map& clusters(){return m_clusters;};
    ImRect& bbox(){return m_bbox;};
    void notify(){ update_count++;};
    bool empty() const { return m_nodes.empty(); };
    void reset() { m_nodes.clear(); m_edges.clear(); m_clusters.clear(); m_node_max_index = 0; m_edge_max_index = 0; m_cluster_max_index = 0; m_clusters[0] = make_shared<cluster_object>(0); };
    void clear() { m_nodes.clear(); m_edges.clear(); m_clusters.clear(); m_clusters[0] = make_shared<cluster_object>(0); };
    bool changed(int &check){ return (update_count != check)&&(check = update_count); } 
    void select_all_node(){ for(auto& n:m_nodes) n.second->m_focused = true; };    
    void select_all_edge(){ for(auto& e:m_edges) e.second->m_focused = true; };    
    void select_all_cluster(){ for(auto& c:m_clusters) c.second->m_focused = true; };  
    void unselect_all_node(){ for(auto& n:m_nodes) n.second->m_focused = false; };
    void unselect_all_edge(){ for(auto& e:m_edges) e.second->m_focused = false; };
    void unselect_all_cluster(){ for(auto& c:m_clusters) c.second->m_focused = false; };
    void toggle_select_node(node n){ n->m_focused = !n->m_focused; };
    void toggle_select_edge(edge e){ e->m_focused = !e->m_focused; };   
    void toggle_select_cluster(cluster c){ c->m_focused = !c->m_focused; }; 
    void select_single_node(node n){ for(auto& i:m_nodes) i.second->m_focused = (i.first == n->m_id); };
    void select_single_edge(edge e){ for(auto& i:m_edges) i.second->m_focused = (i.first == e->m_id); };
    void select_single_cluster(cluster c){ for(auto& i:m_clusters) i.second->m_focused = (i.first == c->m_id); };
    void lock_focused_node(){ for(auto& n:m_nodes) if(n.second->m_focused) n.second->m_locked = true; };
    void unlock_focused_node(){ for(auto& n:m_nodes) if(n.second->m_focused) n.second->m_locked = false; };
    void generate_random_graph(int n, int m, int c){};
    node leaf(){
        update_node_statistcs();
        return choose_node([](node n){return (n->m_outdeg == 0 && n->m_indeg <= 1);});
    }
    vector<node> ancestors(node n) {
        vector<node> ans;
        ans.push_back(n);
        if( auto es = get_in_edges(n); es.size()) {
            map<int, vector<node>> branchs;
            for(auto e: es){
                vector<node> ns = ancestors(e->m_source);
                branchs[ns.size()] = ns;
            }
            if( branchs.size() ) ans.insert(ans.end(), make_move_iterator(branchs.rbegin()->second.begin()), make_move_iterator(branchs.rbegin()->second.end()));
        }
        return ans;
    }
    vector<node> children(node n) {
        vector<node> ans;
        if(auto es = get_out_edges(n); es.size()){
            for(auto e: es) ans.push_back(e->m_target);
        }
        return ans;
    }
    using branch = vector<pair<int, int>>;//<rank,ids>
    using branchs = vector<branch>;
    branchs descendants(node n) {
        branch br;
        branchs bs;
        std::function<void(int, node)> make_branchs = [&](int rank, node n) {
            br.push_back( make_pair(rank, n->m_seq) );
            if( n->m_outdeg == 0 ){
                bs.push_back(br);
                br.clear();
            }
            else{
                std::vector<edge> edges = get_out_edges(n);
                for(std::vector<edge>::iterator it = edges.begin(); it != edges.end(); ++it){
                    if( edges.size() > 1 ){
                        if(it ==  edges.begin()) bs.push_back(br);
                        br.clear();
                        br.push_back( make_pair(rank, n->m_seq) );
                    }
                    make_branchs(rank+1, ((*it)->m_target)); 
                }
            }
        };
        make_branchs(0, n);
        return bs;
    }
    void update_graph_bounding_box(){
        //scan all nodes
        for ( auto n:m_nodes ) {
            if( n.first == m_nodes.begin()->first ){
                m_bbox = n.second->m_rect;
            }
            else{ 
                m_bbox.Min = ImMin(m_bbox.Min, n.second->m_rect.Min);
                m_bbox.Max = ImMax(m_bbox.Max, n.second->m_rect.Max);
             }
        }
        //scan all edges
        for ( auto e:m_edges ) {
            for (auto p:e.second->m_polyline) {
                m_bbox.Min = ImMin(m_bbox.Min, p);
                m_bbox.Max = ImMax(m_bbox.Max, p);
            }
        }

        //scan all clusters
        for ( auto c:m_clusters ) {
            if( c.first != m_clusters.begin()->first ){
                m_bbox.Min = ImMin(m_bbox.Min, c.second->m_rect.Min);
                m_bbox.Max = ImMax(m_bbox.Max, c.second->m_rect.Max);
            }
        }
    };
    void loop(){
        update_node_statistcs();
    };
    void debug(){
        ImGui::Begin("debug");
        ImGui::Text("node count: %d", m_nodes.size());
        ImGui::Text("edge count: %d", m_edges.size());
        ImGui::Text("cluster count: %d", m_clusters.size());
        ImGui::Text("update count: %d", update_count);
        ImGui::BeginChild("promote", ImVec2(-1.0f, 300.0f));
        string content;
        if(node root = choose_focused_node(); root){
            string centrel_topic = root->m_label;
            util::replace_all(centrel_topic, " ", "_");
            util::replace_all(centrel_topic, "\n", "_");
            util::lower_case(centrel_topic);
            using namespace inja;
            using json = nlohmann::json;
            try{
                auto ans = ancestors(root);
                auto des = descendants(root);
                for (size_t i = 0; i < des.size(); i++)
                {
                    ImGui::TextUnformatted(fmt::format("branch{}:{}", i, des[i]).c_str());
                }
                vector<string> ns;
                for(auto x: ans) ns.push_back(x->m_label);
                json data;
                data["keyword"] = centrel_topic;
                data["historys"] = json::array();
                for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
                    if(*it != root->label()) {
                        util::replace_all(*it, " ", "_");
                        util::replace_all(*it, "\n", "_");
                        util::lower_case(*it);
                        data["historys"].push_back(*it);
                    }
                }
                data["history_size"] = data["historys"].size();
                Environment env;
                Template temp = env.parse_template(".//templates//content.txt");
                content = env.render(temp, data);
            }catch(const std::exception& e){}
        }
        float num = std::ceil(ImGui::GetContentRegionAvail().x / ImGui::GetFontSize());
        util::wrap_word(content, num);
        ImGui::TextUnformatted(content.c_str());ImGui::SameLine();
        if(ImGui::Button("copy")) {
            ImGui::LogToClipboard();
            ImGui::LogText(content.c_str());
            ImGui::LogFinish();
        }
        ImGui::EndChild();
        for(auto j: m_weak_nodes){
            if (j.second.expired())
                ImGui::TextColored(ImColor(1.0f,0.0f,0.0f), fmt::format("Node{}: expired!", j.first).c_str());
            else
                ImGui::TextColored(ImColor(0.0f,1.0f,1.0f), fmt::format("Node{}: count: {}", j.second.lock()->m_id, j.second.use_count()).c_str());
        }
        for (auto j: m_weak_edges){
            if (j.second.expired())
                ImGui::TextColored(ImColor(1.0f,0.0f,0.0f), fmt::format("Edge{}: expired!", j.first).c_str());
            else
                ImGui::TextColored(ImColor(0.0f,1.0f,0.0f), fmt::format("Edge{}: count: {}", j.second.lock()->m_id, j.second.use_count()).c_str());
        }
        for (auto j: m_weak_clusters){
            if (j.second.expired())
                ImGui::TextColored(ImColor(1.0f,0.0f,0.0f), fmt::format("Cluster{}: expired!", j.first).c_str());
            else
                ImGui::TextColored(ImColor(0.0f,0.0f,1.0f), fmt::format("Cluster{}: count: {}", j.second.lock()->m_id, j.second.use_count()).c_str());
        }
        ImGui::End();
    }
    //update node statistics
    void update_node_statistcs(){
        static int last_edge_max_index = 0;
        if( last_edge_max_index != m_edge_max_index ){
            last_edge_max_index = m_edge_max_index;
            //update node degree
            for ( auto& n:m_nodes ){
                n.second->m_indeg = count_if(m_edges.begin(), m_edges.end(), [n](edge_pair e){ return node(e.second->m_target) == n.second; });
                n.second->m_outdeg = count_if(m_edges.begin(), m_edges.end(), [n](edge_pair e){ return node(e.second->m_source) == n.second; });
                n.second->m_deg = n.second->m_indeg + n.second->m_outdeg;
            }
        }
    };
    //update node, edge, cluster sequence
    template<class T>
    void refrash_sequence(map<int, T>& m){
        int n = 0;
        for(auto& i:m){ i.second->m_seq = n++; };
        update_count++;
    };
    node get_node(int key) { 
        if(auto search = m_nodes.find(key); search != m_nodes.end()){
            return search->second;
        }else{
            return nullptr;
        }
    };
    void refrash_sequence_all(){
        refrash_sequence(m_nodes);
        refrash_sequence(m_edges);
        refrash_sequence(m_clusters);
    };
    node choose_node(function<bool(node)>f) {
        auto search = find_if(m_nodes.begin(), m_nodes.end(), [f](node_pair i){ return f(i.second); });
        return search!= m_nodes.end()?search->second:nullptr;
    };
    node choose_focused_node() {
        auto search = find_if(m_nodes.begin(), m_nodes.end(), [](node_pair i){ return i.second->m_focused; });
        return search!= m_nodes.end()?search->second:nullptr;
    };
    vector<node> choose_focused_nodes() {
        vector<node> p_focused_nodes;
        for(auto i:m_nodes) if( i.second->m_focused ) p_focused_nodes.push_back(i.second);
        return p_focused_nodes;
    };
    int count_focused_node() { return count_if(m_nodes.begin(), m_nodes.end(), [](node_pair i){ return i.second->m_focused; });};
    int count_focused_edge() { return count_if(m_edges.begin(), m_edges.end(), [](edge_pair e){ return e.second->m_focused; });};
    int count_focused_cluster() { return count_if(m_clusters.begin(), m_clusters.end(), [](cluster_pair c){ return c.second->m_focused; });};
    node insert_node() {
        m_node_max_index++;
        m_nodes[m_node_max_index] = make_shared<node_object>(m_node_max_index);
        m_weak_nodes[m_node_max_index] = m_nodes[m_node_max_index];
        refrash_sequence(m_nodes);
        return m_nodes[m_node_max_index];
    };
    void insert_node(node n){
        m_nodes[n->m_id] = n;
        refrash_sequence(m_nodes);
    }
    //erase a node
    //1.erase all edges connected to the node
    //2.remove node from cluster's node-list if the node is in a cluster
    //3.erase cluster if the cluster's node-list is empty
    //4.erase the node
    node_map::iterator erase_node(node_map::iterator it){
        for(edge_map::iterator eit = m_edges.begin(); eit!= m_edges.end();){
            if(node(eit->second->m_target)->m_id == it->first || node(eit->second->m_source)->m_id == it->first){
                eit = m_edges.erase(eit);
            }else{
                ++eit;
            }
        }
        for(cluster_map::iterator cit = m_clusters.begin(); cit!= m_clusters.end();){
            for(vector<shared_ptr<node_object>>::iterator nit = cit->second->m_child_nodes.begin(); nit!= cit->second->m_child_nodes.end();){
                if(node(*nit)->m_id == it->first){
                    nit = cit->second->m_child_nodes.erase(nit);
                    break;
                }else{
                    ++nit;
                }
            }
            if(cit->second->m_child_nodes.size() == 0 && cit->first != 0){
                cit = m_clusters.erase(cit);
            }else{
                ++cit;
            }
        }
        auto ret = m_nodes.erase(it);
        refrash_sequence(m_nodes);
        refrash_sequence(m_edges);
        refrash_sequence(m_clusters);
        return ret;
    };
    void erase_node(int key){
        auto it = find_if(m_nodes.begin(), m_nodes.end(),[key](node_pair i){ return key == i.second->m_id; });
        if( it != m_nodes.end() ){ erase_node(it); }
    }
    // get a edge
    edge get_edge(int key){
        if(auto search = m_edges.find(key); search != m_edges.end()){
            return search->second;
        }else{
            return nullptr;
        }
    };
    // get all out edges of a node
    vector<edge> get_out_edges(node n){
        vector<edge> out_edges;
        for(auto i:m_edges){ if(node(i.second->m_source) == n) out_edges.push_back(i.second); }
        return out_edges;
    };
    // get all in edges of a node
    vector<edge> get_in_edges(node n){
        vector<edge> in_edges;
        for(auto i:m_edges){ if(node(i.second->m_target) == n) in_edges.push_back(i.second); }
        return in_edges;
    };

    edge choose_edge(function<bool(edge)>f) {
        auto search = find_if(m_edges.begin(), m_edges.end(), [&](edge_pair i){ return f(i.second); });
        return search!= m_edges.end()?search->second:nullptr;
    };
    edge choose_focused_edge() {
        auto search = find_if(m_edges.begin(), m_edges.end(), [](edge_pair i){ return (i.second->m_focused); });
        return search!= m_edges.end()?search->second:nullptr;
    };
    //choose all focused edge
    vector<edge>  choose_focused_edges() {
        vector<edge> p_focused_edges;
        for(auto i:m_edges){ if( i.second->m_focused )p_focused_edges.push_back(i.second); }
        return p_focused_edges;
    };
    // insert a new edge
    edge insert_edge(node source, node target) {
        m_edge_max_index++;
        m_edges[m_edge_max_index] = make_shared<edge_object>(m_edge_max_index, source, target);
        m_weak_edges[m_edge_max_index] = m_edges[m_edge_max_index];
        refrash_sequence(m_edges);
        return m_edges[m_edge_max_index];
    };
    void insert_edge(edge e){
        m_edges[e->m_id] = e;
        refrash_sequence(m_edges);
    }
    // erase a edge
    edge_map::iterator erase_edge(edge_map::iterator it){
        auto ret = m_edges.erase(it);
        refrash_sequence(m_edges);
        return ret;
    };
    void erase_edge(int key){
        auto it = find_if(m_edges.begin(), m_edges.end(),[key](edge_pair i){ return key == i.second->m_id; });
        if( it != m_edges.end() ){ erase_edge(it); }
    }
    //swap edge source and target
    void reverse_edge(edge e){
        assert(e != nullptr);
        auto tmp = e->m_source;
        e->m_source = e->m_target;
        e->m_target = tmp;
        refrash_sequence(m_edges);
    };
    //clear edge's bendpoint
    void clear_bendpoint(edge e){
        assert(e != nullptr);
        e->m_bends.clear();
        refrash_sequence(m_edges);
    }
    //clear all edge's bendpoint
    void clear_all_bendpoint(){
        for( auto e: m_edges ) e.second->m_bends.clear();
        refrash_sequence(m_edges);
    }
    //get a cluster
    cluster get_cluster(int key) {
        if(auto search = m_clusters.find(key); search != m_clusters.end()){
            return search->second;
        }else{
            return nullptr;
        }
    };

    cluster root_cluster() { return m_clusters.at(0); };

    cluster choose_cluster(function<bool(cluster)>f) {
        auto search = find_if(m_clusters.begin(), m_clusters.end(), [f](cluster_pair i){ return f(i.second); });
        return search!= m_clusters.end()?search->second:nullptr;
    };

    cluster choose_focused_cluster() {
        auto search = find_if(m_clusters.begin(), m_clusters.end(), [](cluster_pair i){ return (i.second->m_focused); });
        return search!= m_clusters.end()?search->second:nullptr;
    };
    vector<cluster> choose_focused_clusters() {
        vector<cluster> cs;
        for( auto i: m_clusters ){ if(i.second->m_focused)cs.push_back(i.second); }
        return cs;
    };
    //insert a new cluster
    cluster insert_cluster() {
        m_cluster_max_index++;
        m_clusters[m_cluster_max_index] = make_shared<cluster_object>(m_cluster_max_index);
        m_weak_clusters[m_cluster_max_index] = m_clusters[m_cluster_max_index];
        refrash_sequence(m_clusters);
        return m_clusters[m_cluster_max_index];
    };
    void insert_cluster(cluster c){
        m_clusters[c->m_id] = c;
        refrash_sequence(m_clusters);
    }
    //erase a cluster
    cluster_map::iterator erase_cluster(cluster_map::iterator it){
        auto ret = m_clusters.erase(it);
        refrash_sequence(m_clusters);
        return ret;
    };
    void erase_cluster(int key){
        auto it = find_if(m_clusters.begin(), m_clusters.end(),[key](cluster_pair i){return key == i.second->m_id;});
        if( it!= m_clusters.end() ){
            erase_cluster(it);
        }
    }


};//class Graph


};//namespace mindmap_graph

