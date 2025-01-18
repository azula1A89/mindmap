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
#include "main.h"
#include <base64.hpp>
#include "json.hpp"
#include "inja.hpp"
#include <httplib.h>

namespace mindmap_command
{
using namespace mindmap;

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual string name() = 0;
    virtual void post_process(){};
    string messages;
};

class CreateNodeCommand : public Command {
public:
    explicit CreateNodeCommand(Graph& graph, ImVec2 center, function<void(node)>f) : m_graph(graph), m_node_center(center), m_post_process(f) {}
    void execute() override {
        m_node_inserted = m_graph.insert_node();
    }
    void undo() override {
        m_graph.erase_node(m_node_inserted->m_id);
    }
    void redo() override {
        m_graph.nodes()[m_node_inserted->m_id] = m_node_inserted;
        m_graph.notify();
    }
    std::string name() override {
        return "Create Node";
    }
    void post_process() override {
        m_post_process(m_node_inserted);
    }

private:
    Graph& m_graph;
    ImVec2 m_node_center;
    node m_node_inserted;
    function<void(node)> m_post_process;
};

class DeleteNodeCommand : public Command {
public:
    explicit DeleteNodeCommand(Graph& graph, node node) : m_graph(graph), m_node_deleted(node) {}
    void execute() override {
        int key = m_node_deleted->m_id;
        auto it = find_if(m_graph.nodes().begin(), m_graph.nodes().end(),[key](node_pair i){ return key == i.second->m_id; });
        if( it != m_graph.nodes().end() ){
            //save edge(s) to be delete
            m_edges_deleted.clear();
            for(edge_map::iterator eit = m_graph.edges().begin(); eit!= m_graph.edges().end();){
                if(eit->second->m_target->m_id == it->first || eit->second->m_source->m_id == it->first){
                    m_edges_deleted.push_back(eit->second);
                    eit = m_graph.edges().erase(eit);
                }else{
                    ++eit;
                }
            }
            m_cluster_deleted.clear();
            for(cluster_map::iterator cit = m_graph.clusters().begin(); cit!= m_graph.clusters().end();){
                for(vector<shared_ptr<node_object>>::iterator nit = cit->second->m_child_nodes.begin(); nit!= cit->second->m_child_nodes.end();){
                    if((*nit)->m_id == it->first){
                        nit = cit->second->m_child_nodes.erase(nit);
                        break;
                    }else{
                        ++nit;
                    }
                }
                if(cit->second->m_child_nodes.size() == 0 && cit->first != 0){
                    m_cluster_deleted.push_back(cit->second);
                    cit = m_graph.clusters().erase(cit);
                }else{
                    ++cit;
                }
            }
            m_graph.nodes().erase(it);
            m_graph.notify();
        }
    }
    void undo() override {
        m_graph.nodes()[m_node_deleted->m_id] = m_node_deleted;
        //restore edge(s) and cluster(s)
        for(auto i:m_edges_deleted){
            m_graph.edges()[i->m_id] = i;
        }
        for(auto i:m_cluster_deleted){
            m_graph.clusters()[i->m_id] = i;
        }
        m_graph.notify();
    }
    void redo() override {
        execute();
    }
    std::string name() override {
        return "Delete Node";
    }
private:
    Graph& m_graph;
    node m_node_deleted;
    vector<edge> m_edges_deleted;
    vector<cluster> m_cluster_deleted;
};

class CreateEdgeCommand : public Command {
public:
    explicit CreateEdgeCommand(Graph& graph, node source, node target, function<void(edge)>f) : m_graph(graph), m_source(source), m_target(target),m_post_process(f) {}
    void execute() override {
        m_edge_inserted = m_graph.insert_edge(m_source, m_target);
    }
    void undo() override {
        m_graph.erase_edge(m_edge_inserted->m_id);
    }
    void redo() override {
        m_graph.edges()[m_edge_inserted->m_id] = m_edge_inserted;
        m_graph.notify();
    }
    std::string name() override {
        return "Create Edge";
    }
    void post_process() override {
        m_post_process(m_edge_inserted);
    }
private:
    Graph& m_graph;
    node m_source;
    node m_target;
    edge m_edge_inserted;
    function<void(edge)> m_post_process;
};

class DeleteEdgeCommand : public Command {
public:
    explicit DeleteEdgeCommand(Graph& graph, edge e) : m_graph(graph), m_edge_deleted(e) {}
    void execute() override {
        m_graph.erase_edge(m_edge_deleted->m_id);
    }
    void undo() override {
        m_graph.edges()[m_edge_deleted->m_id] = m_edge_deleted;
        m_graph.notify();
    }
    void redo() override {
        execute();
    }
    std::string name() override {
        return "Delete Edge";
    }
private:
    Graph& m_graph;
    edge m_edge_deleted;
};

class CreateClusterCommand : public Command {
public:
    explicit CreateClusterCommand(Graph& graph, vector<node> nodes, function<void(cluster)> f) : m_graph(graph), m_nodes(nodes), m_post_process(f) {}
    void execute() override {
        m_cluster_created = m_graph.insert_cluster();
        for(auto i:m_nodes){ m_cluster_created->m_child_nodes.push_back(i); }
    }
    void undo() override {
        m_graph.erase_cluster(m_cluster_created->m_id);
    }
    void redo() override {
        m_graph.insert_cluster(m_cluster_created);
        m_graph.notify();
    }
    std::string name() override {
        return "Create Cluster";
    }
    void post_process() override {
        m_post_process(m_cluster_created);
    }
private:
    Graph& m_graph;
    vector<node> m_nodes;
    cluster m_cluster_created;
    function<void(cluster)> m_post_process;
};

class DeleteClusterCommand : public Command {
public:
    explicit DeleteClusterCommand(Graph& graph, cluster c) : m_graph(graph), m_cluster_deleted(c){}
    void execute() override {
        m_graph.erase_cluster(m_cluster_deleted->m_id);
    }
    void undo() override {
        m_graph.clusters()[m_cluster_deleted->m_id] = m_cluster_deleted;
        m_graph.notify();
    }
    void redo() override {
        execute();
    }
    std::string name() override {
        return "Delete Cluster";
    }
private:
    Graph& m_graph;
    cluster m_cluster_deleted;
};

class DeleteSelectedCommand : public Command {
public:
    explicit DeleteSelectedCommand(Graph& graph) : m_graph(graph) {
        m_nodes_deleted.clear();
        m_edges_deleted.clear();
        m_clusters_deleted.clear();
        for(auto i: m_graph.nodes()){
            if(i.second->m_focused){
                m_nodes_deleted.push_back(i.second);
            }
        }
        for(auto i: m_graph.edges()){
            if(i.second->m_focused){
                m_edges_deleted.push_back(i.second);
            }
        }
        for(auto i: m_graph.clusters()){
            if(i.second->m_focused){
                m_clusters_deleted.push_back(i.second);
            }
        }
    }
    void execute() override {
        if( m_nodes_deleted.size() == 0 && m_edges_deleted.size() == 0 && m_clusters_deleted.size() == 0 ) return;
        for( auto it = m_graph.nodes().begin(); !m_graph.nodes().empty() && it != m_graph.nodes().end();){
            if( it->second->m_focused ){
                //find edge(s) that connected to this node and save to be delete
                for (auto eit = m_graph.edges().begin(); eit != m_graph.edges().end();){
                    if(eit->second->m_target->m_id == it->first || eit->second->m_source->m_id == it->first){
                        if (auto x = find_if(m_edges_deleted.begin(), m_edges_deleted.end(), [eit](edge e){return (e->m_id == eit->first);}); x == m_edges_deleted.end()) {
                            m_edges_deleted.push_back(eit->second);
                        }
                        eit = m_graph.edges().erase(eit);
                    }else{
                        ++eit;
                    }
                }
                
                for(cluster_map::iterator cit = m_graph.clusters().begin(); cit!= m_graph.clusters().end(); ){
                    for(vector<shared_ptr<node_object>>::iterator nit = cit->second->m_child_nodes.begin(); nit!= cit->second->m_child_nodes.end();){
                        if((*nit)->m_id == it->first){
                            m_clusters_child_nodes_deleted[cit->first].push_back(*nit);
                            cit->second->m_child_nodes.erase(nit);
                            break;
                        }else{
                            ++nit;
                        }
                    }
                    if(cit->second->m_child_nodes.size() == 0 && cit->second->m_id != 0){
                        m_clusters_deleted.push_back(cit->second);
                        cit = m_graph.clusters().erase(cit);
                    }else{
                        ++cit;
                    }
                }
                it = m_graph.nodes().erase(it);
                m_graph.refrash_sequence_all();
            }else{
                ++it;
            }
        };
        for( auto it = m_graph.edges().begin(); !m_graph.edges().empty() && it !=m_graph.edges().end();){
            if( it->second->m_focused ){
                it = m_graph.erase_edge(it);
            }else{
                ++it;
            }
        };
        for( auto it = m_graph.clusters().begin(); !m_graph.clusters().empty() && it != m_graph.clusters().end();){
            if( it->second->m_focused ){
                it = m_graph.erase_cluster(it);
            }else{
                ++it;
            }
        };
    }
    void undo() override {
        //restore node(s) edge(s) and cluster(s)
        for(auto i:m_nodes_deleted){
            m_graph.insert_node(i);
        }
        for(auto i:m_edges_deleted){
            m_graph.insert_edge(i);
        }
        for(auto i:m_clusters_deleted){
            m_graph.insert_cluster(i);
        }
        for(auto& i:m_graph.clusters()){
            if (m_clusters_child_nodes_deleted.find(i.first) != m_clusters_child_nodes_deleted.end()) {
                for (auto& n : m_clusters_child_nodes_deleted[i.first]) {
                    i.second->m_child_nodes.push_back(n);
                }
            }
        }
    }
    void redo() override {
        for(auto& i:m_graph.clusters()){
            if (m_clusters_child_nodes_deleted.find(i.first) != m_clusters_child_nodes_deleted.end()) {
                for (auto& n : m_clusters_child_nodes_deleted[i.first]) {
                    for(auto it = i.second->m_child_nodes.begin(); it != i.second->m_child_nodes.end();){
                        if((*it)->m_id == n->m_id){
                            it = i.second->m_child_nodes.erase(it);
                        }else{
                            ++it;
                        }
                    }
                }
            }
        }
        for(auto i:m_clusters_deleted){
            m_graph.erase_cluster(i->m_id);
        }
        for(auto i:m_edges_deleted){
            m_graph.erase_edge(i->m_id);
        }
        for(auto i:m_nodes_deleted){
            m_graph.erase_node(i->m_id);
        }
    }
    std::string name() override {
        return "Delete Selected";
    }
private:
    Graph& m_graph;
    vector<node> m_nodes_deleted;
    vector<edge> m_edges_deleted;
    vector<cluster> m_clusters_deleted;
    map<int, vector<shared_ptr<node_object>>> m_clusters_child_nodes_deleted;
};

class MovingNodeCommand : public Command {
public:
    explicit MovingNodeCommand(Graph& graph, vector<node> ns, ImVec2& from,  ImVec2& to) : m_graph(graph), m_nodes_moved(ns) { m_delta =  to - from; }
    void execute() override {}//do nothing here
    void undo() override {
        for( auto i: m_nodes_moved ){ i->m_rect.Translate(-m_delta); }
        m_graph.notify();
    }
    void redo() override {
        for( auto i: m_nodes_moved ){ i->m_rect.Translate(m_delta); }
        m_graph.notify();
    }
    std::string name() override {
        return "Move Node";
    }
private:
    Graph& m_graph;
    vector<node> m_nodes_moved;
    ImVec2 m_delta;
};

class AutoLayoutCommand : public Command {
public:
    explicit AutoLayoutCommand(Graph& graph) : m_graph(graph) {
         m_to.clear();
         m_from.clear();
        for( auto n: m_graph.nodes() ){
            m_from.push_back(n.second->m_rect.GetCenter());
        }
    }
    explicit AutoLayoutCommand(Graph& graph, vector<ImVec2>& to) : m_graph(graph), m_to(to) {
         m_from.clear();
        for( auto n: m_graph.nodes() ){
            m_from.push_back(n.second->m_rect.GetCenter());
        }
    }
    void execute() override {
        assert(m_to.size() == m_from.size());
        for( size_t i = 0; i < m_graph.nodes().size(); i++ ){
            m_delta.push_back(m_to[i] - m_from[i]);
        }
    }
    void undo() override {
        int i = 0;
        for( auto n: m_graph.nodes() ){
            n.second->m_rect.Translate(-m_delta[i]);
            i++;
        }
        m_graph.notify();
    }
    void redo() override {
        int i = 0;
        for( auto n: m_graph.nodes() ){
            n.second->m_rect.Translate(m_delta[i]);
            i++;
        }
        m_graph.notify();
    }
    std::string name() override {
        return "Auto Layout";
    }
    void update(vector<ImVec2>& to) { m_to = to; }
private:
    Graph& m_graph;
    vector<ImVec2> m_from;
    vector<ImVec2> m_to;
    vector<ImVec2> m_delta;
};

class MovingClusterCommand : public Command {
public:
    explicit MovingClusterCommand(Graph& graph, vector<cluster> cs, ImVec2 from,  ImVec2 to) : m_graph(graph), m_clusters_moved(cs), m_from(from), m_to(to) {}
    void execute() override { m_delta =  m_to - m_from; }//do nothing here
    void undo() override {
        for( auto& i: m_clusters_moved ){ 
            for( auto& n: i->m_child_nodes ){
                n->m_rect.Translate(-m_delta); 
            }
        }
        m_graph.notify();
    }
    void redo() override {
        for( auto& i: m_clusters_moved ){
            for( auto& n: i->m_child_nodes ){
                n->m_rect.Translate(m_delta); 
            }
        }
        m_graph.notify();
    }
    std::string name() override {
        return "Move Cluster";
    }
private:
    Graph& m_graph;
    vector<cluster> m_clusters_moved;
    ImVec2 m_from;
    ImVec2 m_to;
    ImVec2 m_delta;
};

template <typename Object, typename Attribute, typename AttributeValueType>
class SetColorCommand : public Command {
public: 
    explicit SetColorCommand(Object obj, Attribute key, AttributeValueType origin_val, AttributeValueType new_val) : m_object(obj), m_key(key), m_value(new_val), m_original_value(origin_val){};
    void execute() override {}
    void undo() override { set_value(m_original_value); }
    void redo() override { set_value(m_value); }
    std::string name() override {
        return "Set Color";
    }
private:
    Object m_object;
    Attribute m_key;
    AttributeValueType m_value;
    AttributeValueType m_original_value;
    void set_value(AttributeValueType val){
        switch (m_key)
        {
        case object_color_t::bd_color:
            m_object->bd_col() = val;
            break;
        case object_color_t::bg_color:
            m_object->bg_col() = val;
            break;
        case object_color_t::txt_color:
            m_object->txt_col() = val;
            break;
        default:
            break;
        }
    }
};

template <typename Object>
class SetLabelCommand : public Command {
public: 
    explicit SetLabelCommand(Graph& graph, Object obj, string origin_val, string new_val) : m_graph(graph), m_object(obj), m_value(new_val), m_original_value(origin_val){};
    void execute() override { set_value(m_value); }
    void undo() override { set_value(m_original_value); }
    void redo() override { set_value(m_value); }
    std::string name() override { return "Set Label"; }
    void update(string& new_val) { m_value = new_val; }
private:
    Graph& m_graph;
    Object m_object;
    string m_value;
    string m_original_value;
    void set_value(string& val){
        m_object->m_label = val;
        m_graph.notify();
    }
};

template <typename Object>
class SetImageCommand : public Command {
public: 
    explicit SetImageCommand(Graph& graph, Object obj, texture_t& origin_val, texture_t& new_val) : m_graph(graph), m_object(obj), m_value(new_val), m_original_value(origin_val){};
    void execute() override { set_value(m_value); }
    void undo() override { set_value(m_original_value); }
    void redo() override { set_value(m_value); }
    std::string name() override { return "Set Image"; }
    void update(texture_t& new_val) { m_value = new_val; }
private:
    Graph& m_graph;
    Object m_object;
    texture_t m_value;
    texture_t m_original_value;
    void set_value(texture_t& val){
        m_object->m_image = val.first;
        m_object->m_image_rect.Min = ImVec2(0, 0);
        m_object->m_image_rect.Max = ImVec2(val.second.x, val.second.y);
        m_graph.notify();
    }
};

//batch set image for selected nodes.
template <typename Object>
class BatchSetImageCommand : public Command {
public: 
    explicit BatchSetImageCommand(Graph& graph, texture_t& new_val) : m_graph(graph), m_value(new_val){
        m_nodes.clear();
        for(auto i: m_graph.nodes()){
            if(i.second->m_focused){
                m_nodes.push_back(i.second);
            }
        }
        m_original_value.clear();
        for(auto i: m_nodes){
            m_original_value.push_back({i->m_image, i->m_image_rect.GetSize()});
        }
    };
    void execute() override { set_value(m_value); }
    void undo() override { restore_value(); }
    void redo() override { set_value(m_value); }
    std::string name() override { return "Batch Set Image"; }
private:
    Graph& m_graph;
    texture_t m_value;
    vector<node> m_nodes;
    vector<texture_t> m_original_value;
    void set_value(texture_t& val){
        for(auto i: m_nodes){
            i->m_image = val.first;
            i->m_image_rect.Min = ImVec2(0, 0);
            i->m_image_rect.Max = ImVec2(val.second.x, val.second.y);
        }
        m_graph.notify();
    }
    void restore_value(){
        for(size_t i = 0; i < m_nodes.size(); i++){
            m_nodes[i]->m_image = m_original_value[i].first;
            m_nodes[i]->m_image_rect.Min = ImVec2(0, 0);
            m_nodes[i]->m_image_rect.Max = ImVec2(m_original_value[i].second);
        }
        m_graph.notify();
    }
};

template <typename Object>
class ResizeImageRectCommand : public Command {
public:
    explicit ResizeImageRectCommand(Graph& graph, Object obj, ImVec2 delta) : m_graph(graph), m_object(obj), m_delta(delta){};
    void execute() override {}
    void undo() override { set_value(-m_delta); }
    void redo() override { set_value(m_delta); }
    std::string name() override { return "Resize Image"; }
private:
    Graph& m_graph;
    Object m_object;
    ImVec2 m_delta;
    void set_value(ImVec2 val){
        m_object->m_image_rect.Max += val;
        m_graph.notify();
    }
};

class CanvasCommand : public Command {
public:
    explicit CanvasCommand(ImGuiEx::Canvas& canvas, ImGuiEx::CanvasView last_view, ImGuiEx::CanvasView view) : m_canvas(canvas) ,m_last_view(last_view), m_view(view) {}
    void execute() override {}
    void undo() override { m_canvas.SetView(m_last_view); }
    void redo() override { m_canvas.SetView(m_view); }
    std::string name() override { return "Canvas Command"; }
private:
    ImGuiEx::Canvas& m_canvas;
    ImGuiEx::CanvasView m_view;
    ImGuiEx::CanvasView m_last_view;
};

//pack selected nodes and edges into a json string and copy to clipboard
class CopyToClipboardCommand : public Command {
public:
    explicit CopyToClipboardCommand(Graph& graph) : m_graph(graph) { };
    void execute() override {
        if( m_graph.count_focused_node() == 0 ) return;
        using json = nlohmann::json;
        json j;
        j["nodes"] = json::array();
        j["edges"] = json::array();
        auto ns = m_graph.choose_focused_nodes();
        for(auto i: ns){
            if( i->m_focused ){
                json node;
                node["id"] = i->m_id;
                node["label"] = i->m_label;
                node["rect"] = {i->m_rect.Min.x, i->m_rect.Min.y, i->m_rect.Max.x, i->m_rect.Max.y};
                node["bd_col"] = i->m_bd_col;
                node["bg_col"] = i->m_bg_col;
                node["txt_col"] = i->m_txt_col;
                node["image"] = i->m_image;
                node["image_rect"] = {i->m_image_rect.Min.x, i->m_image_rect.Min.y, i->m_image_rect.Max.x, i->m_image_rect.Max.y};
                j["nodes"].push_back(node);
            }
        }
        
        for(auto i: m_graph.edges()){
            if( i.second->m_source->m_focused && i.second->m_target->m_focused ){
                json edge;
                edge["id"] = i.second->m_id;
                edge["label"] = i.second->m_label;
                edge["source"] = i.second->m_source->m_id;
                edge["target"] = i.second->m_target->m_id;
                edge["bends"] = json::array();
                for(auto& b: i.second->m_bends){
                    edge["bends"].push_back({b.x, b.y});
                }
                edge["label_position"] = i.second->m_label_position;
                edge["edge_col"] = i.second->m_edge_col;
                edge["txt_col"] = i.second->m_txt_col;
                j["edges"].push_back(edge);
            }
        }
        ImGui::LogToClipboard();
        ImGui::LogText(j.dump().c_str());
        ImGui::LogFinish();
    }
    void undo() override {}
    void redo() override { execute(); }
    std::string name() override { return "Copy To Clipboard"; }
private:
    Graph& m_graph;
};

//try parse json string from clipboard
//then recreate nodes and edges
class PasteFromClipboardCommand : public Command {
public:
    explicit PasteFromClipboardCommand(Graph& graph, ImVec2 center) : m_graph(graph), m_center(center), m_nodes(), m_edges() { };
    void execute() override {
        using json = nlohmann::json;
        json j;
        try{
            j = json::parse(ImGui::GetClipboardText());
        }catch(...){
            return;
        }
        if( j.find("nodes") == j.end() || j.find("edges") == j.end() ) return;
        int node_id_offset = m_graph.node_max_index();
        int edge_id_offset = m_graph.edge_max_index();
        int cluster_id_offset = m_graph.cluster_max_index();
        m_graph.unselect_all_node();
        for(auto& i: j["nodes"]){
            ImVec2 min(i["rect"][0], i["rect"][1]);
            ImVec2 max(i["rect"][2], i["rect"][3]);
            int id = i["id"];
            node n = make_shared<node_object>(id+node_id_offset);
            m_graph.insert_node(n);
            n->m_label = i["label"];
            n->m_rect = ImRect(min, max);
            n->m_focused = true;
            n->m_bd_col = i["bd_col"];
            n->m_bg_col = i["bg_col"];
            n->m_txt_col = i["txt_col"];
            n->m_image = i["image"];
            n->m_image_rect = ImRect(ImVec2(i["image_rect"][0], i["image_rect"][1]), ImVec2(i["image_rect"][2], i["image_rect"][3]));
            m_nodes.push_back(n);
        }
        if(m_graph.nodes().size())m_graph.node_max_index() = m_graph.nodes().rbegin()->first;
        m_graph.unselect_all_edge();
        for(auto& i: j["edges"]){
            int eid = i["id"];
            int source = i["source"]; source += node_id_offset;
            int target = i["target"]; target += node_id_offset;
            node s = m_graph.get_node(source);
            node t = m_graph.get_node(target);
            edge e = make_shared<edge_object>(eid + edge_id_offset, s, t);
            m_graph.insert_edge(e);
            e->m_label = i["label"];
            e->m_bends.clear();
            for(auto& b: i["bends"]){
                e->m_bends.push_back(ImVec2(b[0], b[1]));
            }
            e->m_label_position = i["label_position"];
            e->m_edge_col = i["edge_col"];
            e->m_txt_col = i["txt_col"];
            m_edges.push_back(e);
        }
        if(m_graph.edges().size()) m_graph.edge_max_index() = m_graph.edges().rbegin()->first;
    };
    void undo() override {
        for(auto i: m_nodes){
            m_graph.erase_node(i->m_id);
        }
        for(auto i: m_edges){
            m_graph.erase_edge(i->m_id);
        }
    }
    void redo() override {
        for(auto i: m_nodes){
            m_graph.insert_node(i);
        }
        for(auto i: m_edges){
            m_graph.insert_edge(i);
        }
    }
    std::string name() override { return "Paste From Clipboard"; }
    void post_process() override {
        //calculate nodes's bounding box and move to center
        if( m_nodes.size() > 0 ){
            ImVec2 min(m_nodes.front()->m_rect.Min);
            ImVec2 max(m_nodes.front()->m_rect.Max);
            for(auto i:m_nodes){
                min = ImMin(min, i->m_rect.Min);
                max = ImMax(max, i->m_rect.Max);
            }
            ImRect bbox(min, max);
            ImVec2 delta = m_center - bbox.GetCenter();
            for(auto i:m_nodes){
                i->m_rect.Translate(delta);
            }
        }
        m_graph.notify();
    }
private:
    Graph& m_graph;
    ImVec2 m_center;
    vector<node> m_nodes;
    vector<edge> m_edges;
};

//save graph to json file
class ExportCommand : public Command {
public:
    explicit ExportCommand(Graph& graph) : m_graph(graph) { };
    void execute() override {
        if( m_graph.nodes().size() == 0 ) return;
        using namespace inja;
        using json = nlohmann::json;
        
        const float graph_padding = 40.0f;
        m_graph.update_graph_bounding_box();
        json j;
        j["graph_padding"] = graph_padding;
        j["node_count"] = m_graph.nodes().size();
        j["edge_count"] = m_graph.edges().size();
        j["cluster_count"] = m_graph.clusters().size()-1;//not include root cluster
        j["x0"] = m_graph.bbox().Min.x;
        j["y0"] = m_graph.bbox().Min.y;
        j["width"] = ceilf(m_graph.bbox().GetWidth());
        j["height"] = ceilf(m_graph.bbox().GetHeight());
        j["background_color"] = util::ImU32toHex(ImU32(ImColor(ImGui::GetStyle().Colors[ImGuiCol_WindowBg])));
        j["nodes"] = json::array();
        j["edges"] = json::array();
        j["clusters"] = json::array();
        j["images"] = json::array();
        for( auto i: loaded_textures_base64 ){
            json image;
            image["id"] = i.first;
            image["base64"] = i.second.get()->c_str();
            j["images"].push_back(image);
        }
        for(auto n:m_graph.nodes()){
            ImRect rect = n.second->m_rect;
            rect.Translate(-m_graph.bbox().Min + ImVec2(graph_padding,graph_padding));
            json i;
            i["id"] = n.second->m_id;
            i["sequence"] = n.second->m_seq; 
            i["label"] = n.second->m_label; 
            i["font_size"] = n.second->m_label_font_size;
            i["margin"] = n.second->m_label_margin;
            i["label_width"] = n.second->m_label_sz.x;
            i["label_height"] = n.second->m_label_sz.y;
            i["indegree"] = n.second->m_indeg;
            i["outdegree"] = n.second->m_outdeg;
            i["degree"] = n.second->m_deg;
            i["focused"] = n.second->m_focused;
            i["locked"] = n.second->m_locked;
            i["rect_x"] = rect.Min.x;
            i["rect_y"] = rect.Min.y;
            i["rect_width"] = rect.GetWidth();
            i["rect_height"] = rect.GetHeight();
            i["border_color"] = ImU32(n.second->m_bd_col);
            i["border_color_str"] = util::ImU32toHex(n.second->m_bd_col);
            i["background_color"] = ImU32(n.second->m_bg_col);
            i["background_color_str"] = util::ImU32toHex(n.second->m_bg_col);
            i["text_color"] = ImU32(n.second->m_txt_col);
            i["text_color_str"] = util::ImU32toHex(n.second->m_txt_col);
            i["image"] = n.second->m_image;
            i["image_path"] = "";
            i["image_scale"] = 1.0f;
            if( loaded_textures_base64.find(n.second->m_image) != loaded_textures_base64.end() ){
                if(auto search = find_if(loaded_textures.begin(), loaded_textures.end(), [n](auto i){return i.second->first == n.second->m_image;}); search != loaded_textures.end()){
                    i["image_path"] = search->first;
                    i["image_scale"] = n.second->m_image_rect.GetWidth() / search->second->second.x;
                }
                i["image_width"] = n.second->m_image_rect.GetWidth();
                i["image_height"] = n.second->m_image_rect.GetHeight();
            }else{
                i["image_width"] = 0;
                i["image_height"] = 0;
            }
            j["nodes"].push_back(i);
        }
        for(auto e:m_graph.edges()){
            ImRect rect = e.second->m_label_rect;
            rect.Translate(-m_graph.bbox().Min + ImVec2(graph_padding,graph_padding));
            json i;
            i["id"] = e.second->m_id;
            i["sequence"] = e.second->m_seq;
            i["focused"] = e.second->m_focused;
            i["target"] = e.second->m_target->m_id;
            i["source"] = e.second->m_source->m_id;
            i["label"] = e.second->m_label;
            i["font_size"] = e.second->m_label_font_size;
            i["arrow_type"] = e.second->m_arrow;
            i["arrow_ear1_x"] = e.second->m_arrow_ear1.x - m_graph.bbox().Min.x + graph_padding;
            i["arrow_ear1_y"] = e.second->m_arrow_ear1.y - m_graph.bbox().Min.y + graph_padding;
            i["arrow_ear2_x"] = e.second->m_arrow_ear2.x - m_graph.bbox().Min.x + graph_padding;
            i["arrow_ear2_y"] = e.second->m_arrow_ear2.y - m_graph.bbox().Min.y + graph_padding;
            i["arrow_ear3_x"] = e.second->m_arrow_ear3.x - m_graph.bbox().Min.x + graph_padding;
            i["arrow_ear3_y"] = e.second->m_arrow_ear3.y - m_graph.bbox().Min.y + graph_padding;
            i["arrow_ear4_x"] = e.second->m_arrow_ear4.x - m_graph.bbox().Min.x + graph_padding;
            i["arrow_ear4_y"] = e.second->m_arrow_ear4.y - m_graph.bbox().Min.y + graph_padding;
            i["label_position"] = e.second->m_label_position;
            i["ideal_length"] = e.second->m_ideal_length;
            i["label_x"] = rect.Min.x;
            i["label_y"] = rect.Min.y;
            i["label_width"] = rect.GetWidth();
            i["label_height"] = rect.GetHeight();
            i["edge_color"] = e.second->m_edge_col;
            i["edge_color_str"] = util::ImU32toHex(e.second->m_edge_col);
            i["text_color"] = e.second->m_txt_col;
            i["text_color_str"] = util::ImU32toHex(e.second->m_txt_col);
            for(auto b: e.second->m_bends){
                json ii;
                ii["x"] = b.x;
                ii["y"] = b.y;
                i["bends"].push_back(ii);
            }
            for(auto p: e.second->m_polyline){
                json ii;
                ii["x"] = p.x - m_graph.bbox().Min.x + graph_padding;
                ii["y"] = p.y - m_graph.bbox().Min.y + graph_padding;
                i["polyline"].push_back(ii);
            }
            j["edges"].push_back(i);
        }
        for(auto c:m_graph.clusters()){
            if(c.second == m_graph.root_cluster())continue;
            json i;
            ImRect rect = c.second->m_rect;
            ImRect label_rect = c.second->m_label_rect;
            rect.Translate(-m_graph.bbox().Min + ImVec2(graph_padding,graph_padding));
            label_rect.Translate(-m_graph.bbox().Min + ImVec2(graph_padding,graph_padding));
            i["id"] = c.second->m_id;
            i["sequence"] = c.second->m_seq;
            i["focused"] = c.second->m_focused;
            i["margin"] = c.second->m_margin;
            i["padding"] = c.second->m_padding;
            i["label"] = c.second->m_label;
            i["font_size"] = c.second->m_label_font_size;
            i["rect_x"] = rect.Min.x;
            i["rect_y"] = rect.Min.y;
            i["rect_width"] = rect.GetWidth(); 
            i["rect_height"] = rect.GetHeight();
            i["label_x"] = label_rect.Min.x;
            i["label_y"] = label_rect.Min.y;
            i["label_width"] = label_rect.GetWidth();
            i["label_height"] = label_rect.GetHeight();
            i["border_color"] = ImU32(c.second->m_bd_col);
            i["border_color_str"] = util::ImU32toHex(c.second->m_bd_col);
            i["background_color"] = c.second->m_bg_col;
            i["background_color_str"] = util::ImU32toHex(c.second->m_bg_col);
            i["text_color"] = c.second->m_txt_col;
            i["text_color_str"] = util::ImU32toHex(c.second->m_txt_col);
            for(auto n:c.second->m_child_nodes){
                json ii;
                ii["id"] = n->m_id;
                i["nodes"].push_back(ii);
            }
            j["clusters"].push_back(i); 
        }

        if (j.empty()) return;
        try{
            std::ofstream ostrm("output.json", std::ios::binary);
            ostrm.write(j.dump().c_str(), j.dump().size());
        }catch(const std::exception& e){
            messages = fmt::format("while executing ExportCommand:{}", e.what());
        }
        //export svg
        Environment env;
        env.add_callback("split", 2, [](Arguments& args) {
            string str = args.at(0)->get<string>();
            string delimiter = args.at(1)->get<string>();
            size_t pos = 0;
            inja::json token;
            inja::json tokens;
            while ((pos = str.find(delimiter)) != string::npos) {
                token = str.substr(0, pos);
                tokens.push_back(token);
                str.erase(0, pos + delimiter.length());
            }
            tokens.push_back(str);
            return tokens;
        });
        env.add_callback("divide", 2, [](inja::Arguments args) {
            double number1 = args.at(0)->get<double>();
            auto number2 = args.at(1)->get<double>();
            return number1 / number2;
        });
        try{
            Template temp = env.parse_template(".//templates//svg.txt");
            Template temp1 = env.parse_template(".//templates//lunasvg.txt");
            Template temp2 = env.parse_template(".//templates//html.txt");
            Template temp3 = env.parse_template(".//templates//script.txt");
            env.write(temp, j, "output.svg");
            env.write(temp1, j, "output-lunasvg.svg");
            env.write(temp2, j, "output.html");
            env.write(temp3, j, "output.js");
        }catch(const std::exception& e){
            messages = fmt::format("while executing ExportCommand:{}", e.what());
        }
    }
    void undo() override {}
    void redo() override {}
    std::string name() override { return "Export"; }
private:
    Graph& m_graph;
};

//load graph from json file
class ImportCommand : public Command {
public:
    explicit ImportCommand(Graph& graph) : m_graph(graph) { };
    void execute() override {
        using json = nlohmann::json;
        try
        {
            ifstream f("output.json");
            json data = json::parse(f);
            if(data.empty())return;
            int node_id_offset = m_graph.node_max_index();
            int edge_id_offset = m_graph.edge_max_index();
            int cluster_id_offset = m_graph.cluster_max_index();
            m_graph.clear();
            for(auto& j:data["nodes"]){
                int id = j["id"];
                node n = make_shared<node_object>(id + node_id_offset);
                m_graph.insert_node(n);
                n->m_seq = j["sequence"];
                n->m_indeg = j["indegree"];
                n->m_outdeg = j["outdegree"];
                n->m_deg = j["degree"];
                n->m_locked = j["locked"];
                n->m_focused = j["focused"];
                n->m_label = j["label"];
                n->m_bd_col = ImU32(j["border_color"]);
                n->m_bg_col = ImU32(j["background_color"]);
                n->m_txt_col = ImU32(j["text_color"]);
                n->m_rect.Min.x = j["rect_x"];
                n->m_rect.Min.y = j["rect_y"];
                n->m_rect.Max.x = j["rect_width"];
                n->m_rect.Max.y = j["rect_height"];
                n->m_rect.Max.x += n->m_rect.Min.x;
                n->m_rect.Max.y += n->m_rect.Min.y;
                string path = j["image_path"];
                if( !path.empty() ){//try load image from file
                    auto l = load_texture_from_file(path);
                    if( ImLength(l.second)> 0){
                        n->m_image = l.first;
                        n->m_image_rect.Min.x = 0;
                        n->m_image_rect.Min.y = 0;
                        n->m_image_rect.Max.x = j["image_width"];
                        n->m_image_rect.Max.y = j["image_height"];
                        messages.append(fmt::format("load image from file:{}\n", path));
                    }
                    else//try load image from base64 string that stored in json file
                    {
                        string base64 = "";
                        for(auto i : data["images"])
                        {
                            if (i["id"] == j["image"])
                            {
                                base64 = i["base64"];
                                break;
                            }
                        }
                        auto l = load_texture_from_base64(base64);
                        ImTextureID image = j["image"];
                        loaded_textures[path] = make_shared<texture_t>(l);
                        loaded_textures_base64[l.first] = make_shared<string>(base64);
                        if( ImLength(l.second)> 0){
                            n->m_image = l.first;
                            n->m_image_rect.Min.x = 0;
                            n->m_image_rect.Min.y = 0;
                            n->m_image_rect.Max.x = j["image_width"];
                            n->m_image_rect.Max.y = j["image_height"];
                        }
                        messages.append(fmt::format("load image from base64 string.\n"));
                    }
                }else{
                    n->m_image = 0;
                    n->m_image_rect.Min.x = 0;
                    n->m_image_rect.Min.y = 0;
                    n->m_image_rect.Max.x = 0;
                    n->m_image_rect.Max.y = 0;
                }
            }
            if(m_graph.nodes().size()) m_graph.node_max_index() = m_graph.nodes().rbegin()->first;

            for(auto& j:data["edges"]){
                int eid = j["id"];
                int src_id = j["source"];
                int tgt_id = j["target"];
                node src = m_graph.get_node(src_id + node_id_offset);
                node tgt = m_graph.get_node(tgt_id + node_id_offset);
                assert(src != nullptr && tgt != nullptr);
                edge e = make_shared<edge_object>(eid + edge_id_offset, src, tgt);
                m_graph.insert_edge(e);
                e->m_seq = j["sequence"];
                e->m_focused = j["focused"];
                e->m_label = j["label"];
                e->m_arrow = j["arrow_type"];
                e->m_label_position = j["label_position"];
                e->m_ideal_length = j["ideal_length"];
                e->m_edge_col = ImU32(j["edge_color"]); 
                e->m_txt_col =  ImU32(j["text_color"]);
                for(auto& k: j["bends"]){
                    e->m_bends.push_back(ImVec2(k["x"], k["y"]));
                }
                for(auto& k: j["polyline"]){
                    e->m_polyline.push_back(ImVec2(k["x"], k["y"]));
                }
            }
            if(m_graph.edges().size()) m_graph.edge_max_index() = m_graph.edges().rbegin()->first;
            for(auto& j:data["clusters"]){
                int id = j["id"];
                cluster c = make_shared<cluster_object>( id + cluster_id_offset );
                m_graph.insert_cluster(c);
                c->m_seq = j["sequence"];
                c->m_focused = j["focused"];
                c->m_margin = j["margin"];
                c->m_padding = j["padding"];
                c->m_label = j["label"];
                c->m_rect.Min.x = j["rect_x"];
                c->m_rect.Min.y = j["rect_y"];
                c->m_rect.Max.x = j["rect_width"];
                c->m_rect.Max.y = j["rect_height"];
                c->m_rect.Max.x += c->m_rect.Min.x;
                c->m_rect.Max.y += c->m_rect.Min.y;
                c->m_bd_col = ImU32(j["border_color"]);
                c->m_bg_col = ImU32(j["background_color"]);
                c->m_txt_col = ImU32(j["text_color"]);
                for(auto& k: j["nodes"]){
                    int id = k["id"];
                    node pn =  m_graph.get_node(id + node_id_offset);
                    assert(pn != nullptr);
                    c->m_child_nodes.push_back(pn);
                }
            }
            if(m_graph.clusters().size()) m_graph.cluster_max_index() = m_graph.clusters().rbegin()->first;
        }
        catch(const std::exception& e)
        {
            messages = fmt::format("while executing ImportCommand:{}", e.what());
        }
        m_graph.notify();
    }
    void undo() override {}
    void redo() override {}
    std::string name() override { return "Import"; }
private:
    Graph& m_graph;
};

class OllamaServer
{
public:
    using ollama_data = std::vector<std::string>;
    using ollama_result =  std::future<ollama_data>;
    explicit OllamaServer(std::string& ollama_url) : m_ready(false), m_url(ollama_url){
        m_result = async(std::launch::async, [&](){
            using namespace httplib;
            using json = nlohmann::json;
            Client cli(m_url);
            ollama_data ret = {};
            if (auto res = cli.Get("/api/tags")) {
                if (res->status == StatusCode::OK_200) {
                    try {
                        json data = json::parse(res->body);
                        if(!data.empty()) {
                            for( auto m: data["models"] ){
                                ret.push_back(m["name"]);
                            }
                        }
                    }
                    catch(const std::exception& e) {
                        m_message.append(e.what());
                        m_message.append("\n");
                    }
                }
            } else {
                m_message.append(to_string(res.error()));
                m_message.append("\n");
            }
            return ret;
        });
    }
    bool ready() {
        if ( !m_ready && std::future_status::ready == m_result.wait_for(std::chrono::milliseconds(1)) ) 
        {
            m_ready = true;
            m_data = m_result.get();
        }
        return m_ready;
    }
    ollama_data& data() {  return m_data;}
private:
    bool m_ready;
    ollama_result m_result;
    std::string m_url;
    std::string m_message;
    ollama_data m_data;
};

class CallOllamaCommand : public Command
{
public:
    explicit CallOllamaCommand(Graph& graph, string url, string model, function<void(vector<string>&, vector<string>&, node)> f, node root = nullptr) : m_graph(graph), m_url(url), m_model(model), m_post_process(f), m_root_node(root), m_node_labels({}), m_edge_labels({}) { };
    void execute() override {
        if( m_root_node ){}
        else if( m_root_node = m_graph.choose_focused_node(); m_root_node == nullptr ) return;
        string centrel_topic = m_root_node->m_label;
        util::replace_all(centrel_topic, " ", "_");
        util::replace_all(centrel_topic, "\n", "_");
        util::lower_case(centrel_topic);
        using namespace inja;
        using namespace httplib;
        using json = nlohmann::json;
        string content;
        try{
            auto ans = m_graph.ancestors(m_root_node);
            vector<string> ns;
            for(auto x: ans) ns.push_back(x->m_label);
            json data;
            data["keyword"] = centrel_topic;
            data["historys"] = json::array();
            for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
                if(*it != m_root_node->label()) {
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
        }catch(const std::exception& e){
            messages = fmt::format("while executing CallOllamaCommand:{}", e.what());
        }
        Client cli(m_url);
        Headers header;
        json body, msg;
        body["model"] = m_model;
        body["messages"] = json::array();
        msg["role"] = "user";
        msg["content"] = content;
        body["messages"].push_back(msg);
        body["stream"] = false;
        body["options"]["temperature"] = 0;
        // body["options"]["num_ctx"] = 4096;
        body["format"] = u8R"({"type":"object","properties":{"topics":{"type":"array","items":{"type":"object","properties":{"topic":{"type":"string","description":"a short topic name"},"explanation":{"type":"string","description":"a long topic explanation"}},"required":["topic","explanation"]}}}})"_json;
        string dump =body.dump();
        if (auto res = cli.Post("/api/chat", body.dump(), "application/json")) {
            if (res->status == StatusCode::OK_200) {
                try
                {
                    json data = json::parse(res->body);
                    if(!data.empty()){
                        string msg = data["message"]["content"];
                        messages.append(msg);
                        string::size_type s = msg.find("{");
                        string::size_type e = msg.rfind("}");
                        msg = msg.substr(s, e-s+1);
                        json j = json::parse(msg);
                        for(auto i: j["topics"]) {
                            string explanation, topic;
                            explanation = i["explanation"];
                            topic = i["topic"];
                            util::wrap_word(explanation, 1.5f*std::sqrtf(explanation.size()));
                            util::replace_all(topic, "_", " ");
                            util::wrap_word(topic, 20);
                            m_node_labels.push_back(topic);
                            m_edge_labels.push_back(explanation);
                        }
                    }
                }
                catch(const std::exception& e)
                {
                    messages.append(e.what());
                    m_node_labels.clear();
                    m_edge_labels.clear();
                }
            }
        } else {
            auto err = res.error();
            messages.append(httplib::to_string(err));
        }
    }
    void undo() override{}
    void redo() override{}
    std::string name() override { return "Call Ollama"; }
    void post_process() override {
        m_post_process(m_node_labels, m_edge_labels, m_root_node);
    }
private:
    string m_url;
    string m_model;
    Graph& m_graph;
    node m_root_node;
    function<void(vector<string>&, vector<string>&, node)> m_post_process;
    vector<string>  m_node_labels;
    vector<string>  m_edge_labels;
};

class ConstrainedFDLayoutCommand : public Command
{
public:
    explicit ConstrainedFDLayoutCommand(Graph& graph) : m_graph(graph) {}
    void execute() override {}
    void undo() override {}
    void redo() override {}
    std::string name() override { return "Cola layout"; }
private:
    Graph& m_graph;
};


using NColor = SetColorCommand<node, object_color_t, ImU32>;
using EColor = SetColorCommand<edge, object_color_t, ImU32>;
using CColor = SetColorCommand<cluster, object_color_t, ImU32>;
using NLabel = SetLabelCommand<node>;
using ELabel = SetLabelCommand<edge>;
using CLabel = SetLabelCommand<cluster>;
using NImage = SetImageCommand<node>;
using BNImage = BatchSetImageCommand<node>;
using ImageSZ = ResizeImageRectCommand<node>;
typedef struct ollama_return_s
{
    std::vector<string> nodes;
    std::vector<string> edges;
    node root;
}ollama_return_t;

class GraphEditor {
public:
    GraphEditor() = default;
    void execute_command(std::unique_ptr<Command> command) {
        command->execute();
        command->post_process();
        _undo_stack.push_back(std::move(command));
        _redo_stack = std::vector<std::unique_ptr<Command>>(); 
    }

    void undo() {
        if (_undo_stack.empty()) {
            return;
        }
        auto command = std::move(_undo_stack.back());
        _undo_stack.pop_back();
        command->undo();
        _redo_stack.push_back(std::move(command));
    }

    void redo() {
        if (_redo_stack.empty()) {
            return;
        }
        auto command = std::move(_redo_stack.back());
        _redo_stack.pop_back();
        command->redo();
        _undo_stack.push_back(std::move(command));
    }
    void debug(){
        static int undo_count = 0;
        static int redo_count = 0;
        ImGui::Begin("debug");
        ImGui::TextColored(ImColor(1.0f, 0.0f, 0.0f, 1.0f), "Undo Stack Size: %d", _undo_stack.size());
        ImGui::BeginChild("undo_stack", ImVec2(0.0f, 130.0f));
        for(auto& i: _undo_stack){
            ImGui::TextColored(ImColor(1.0f, 0.0f, 0.0f, 1.0f) ,i->name().c_str());
        }
        if(undo_count != _undo_stack.size()){ undo_count = _undo_stack.size(); ImGui::SetScrollHereY(1.0f); }
        ImGui::EndChild();
        ImGui::TextColored(ImColor(0.0f, 0.0f, 1.0f, 1.0f),"Redo Stack Size: %d", _redo_stack.size());
        ImGui::BeginChild("redo_stack", ImVec2(0.0f, 130.0f));
        for(auto& i: _redo_stack){
            ImGui::TextColored(ImColor(0.0f, 0.0f, 1.0f, 1.0f), i->name().c_str());
        }
        if(redo_count != _redo_stack.size()){ redo_count = _redo_stack.size(); ImGui::SetScrollHereY(1.0f); }
        ImGui::EndChild();
        ImGui::BeginChild("messages", ImVec2(0.0f, 130.0f));
        if( !_undo_stack.empty() && !_undo_stack.back()->messages.empty() ){
            ImGui::InputTextMultiline("##undo_message", &_undo_stack.back()->messages, ImVec2(-FLT_MIN, -1), ImGuiInputTextFlags_None);
        }
        for(auto& i: _undo_stack){
            if(i->messages.empty())continue;
            ImGui::InputTextMultiline("##undo_message", &i->messages, ImVec2(-FLT_MIN, -1), ImGuiInputTextFlags_None);
        }
        for(auto& i: _redo_stack){
            if(i->messages.empty())continue;
            ImGui::InputTextMultiline("##redo_message", &i->messages, ImVec2(-FLT_MIN, -1), ImGuiInputTextFlags_None);
        }
        ImGui::EndChild();
        ImGui::End();
    }
private:
    std::vector<std::unique_ptr<Command>> _undo_stack;
    std::vector<std::unique_ptr<Command>> _redo_stack;
};

} // namespace mindmap_command

