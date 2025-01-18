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
#include "mindmap.h"
#include "imgui_extra_math.h"
#include "imgui_canvas.h"
#include "imgui_stdlib.h"
#include "imgui_markdown.h" 
#include "mindmap_graph.h"
#include "mindmap_graph_layout.h"
#include "mindmap_state_machine.h"
#include "mindmap_command.h"

using namespace std;
using namespace chrono_literals;
using namespace mindmap;
using namespace state_machine;
using namespace mindmap_command;
using namespace mindmap_layout;

void draw()
{    
    ImGui::Begin("MainView"); 
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    
    //canvas begin.
    static Graph G;
    static GraphEditor GE;
    static state_machine_t sm;
    static ImGuiEx::Canvas canvas;
    static alignment_list_t alignments;
    bool canvas_enabled = canvas.Begin("canvas", ImVec2(0,0));
    static ImDrawList *drawlist = ImGui::GetWindowDrawList();
    static ImDrawListSplitter splitter;splitter.Split(drawlist, 5);
    static ImFont* node_font = H2;
    static ImFont* edge_font = H3;
    static ImFont* cluster_font = H1;
    static ImVec2 origin;
    static float canvas_panning_speed(100000);
    static ImVec2 canvas_panning_offset;
    static float canvas_panning_distance;
    static float scale = 1.0f;
    static float node_text_margins = 8;
    static ImVec2 node_sz(120,80);
    static float arrow_head_sz(13);
    static float arrow_head_thickness(1.5);
    static float arrow_line_thickness(2);
    static float arrow_angle(45);
    static float dot_sz(5);
    static float border_thickness = 2.8f;
    static float cluster_padding = 20.0f;
    static float cluster_margin = 50.0f;
    static float ideal_length = 1.0f;
    static float proximity_threshold = 20;
    static float pin_radius = 8;
    static float guideline_search_threshold = 5000;
    static float guideline_snap_threshold = 30;
    static int contact_point_type = 0;
    static float ideal_dot_gap = 100;
    static float flow_speed = 0;
    static int flow_direction = 1;
    static int auto_active_label_editor = 1;//0:none, 1:on double-click, 2:on creat, 3: both
    static float alignment_space = 0.5; 
    static bool want_editing = false;

    static bool layout_once = false;
    static bool route_once = false;
    static int cola_inprogress = 0;
    static bool enable_continue_layout = false;
    static bool enable_continue_route = false;
    static bool enable_tree_mode = false;
    static bool should_update_graph_attributes = false;
    static bool show_debug_window = false;

    const ImU32 window_bg_col = ImColor(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
    static ImColor node_text_col = ImGui::GetStyle().Colors[MindMap_Node_Text];
    static ImColor node_border_col = ImGui::GetStyle().Colors[MindMap_Node_Border];
    static ImColor node_fill_col = ImGui::GetStyle().Colors[MindMap_Node_Bg];
    static ImColor edge_col = ImGui::GetStyle().Colors[MindMap_Edge];
    static ImColor edge_text_col = ImGui::GetStyle().Colors[MindMap_Node_Text];
    static ImColor select_box_fill_col = ImGui::GetStyle().Colors[MindMap_Rect_Bg];
    static ImColor focused_col = ImGui::GetStyle().Colors[MindMap_Focused];
    static ImColor lock_icon_col = ImGui::GetStyle().Colors[MindMap_Lock_Icon];
    static ImColor linking_col = ImGui::GetStyle().Colors[MindMap_Edge_Linking];
    static ImColor cluster_background_col = ImGui::GetStyle().Colors[MindMap_Cluster_Bg];
    static ImColor cluster_focused_col = ImGui::GetStyle().Colors[MindMap_Cluster_Focused];
    static ImColor cluster_text_col = IM_COL32(10,10,10,100);

    const ImGuiInputFlags input_flags = ImGuiInputFlags_RouteFocused;
    const ImGuiKeyChord key_chord_copy = ImGuiMod_Ctrl | ImGuiKey_C;
    const ImGuiKeyChord key_chord_paste = ImGuiMod_Ctrl | ImGuiKey_V;
    const ImGuiKeyChord key_chord_select_all = ImGuiMod_Ctrl | ImGuiKey_A;
    const ImGuiKeyChord key_chord_save = ImGuiMod_Ctrl | ImGuiKey_S;
    const ImGuiKeyChord key_chord_cut = ImGuiMod_Ctrl | ImGuiKey_X;
    const ImGuiKeyChord key_chord_undo = ImGuiMod_Ctrl | ImGuiKey_Z;
    const ImGuiKeyChord key_chord_redo = ImGuiMod_Ctrl | ImGuiKey_Y;
    ImGuiIO& io = ImGui::GetIO();(void)io;
    
    //==============================================Edit logic start==========================================================
    //C = (B · A / ||A||^2) * A
    auto get_projection = [](ImVec2 A, ImVec2 B)->ImVec2{
        float dot_product = ImDot(B, A);
        float magnitude = ImLength(A);
        return (dot_product / (magnitude*magnitude)) * (A);
    };
    auto mouse_proximity_test = [&](ImVec2 p1, ImVec2 p2, float proximity_threshold)->bool{
        bool result = false;
        ImVec2 p = ImGui::GetMousePos();
        ImVec2 v = p2 - p1;
        ImVec2 w = p - p1;
        float dot_product = ImDot(v, w);
        float magnitude = ImLength(v);
        if( magnitude > 1e-4f && dot_product > 0){
            float projection_length = ImAbs(dot_product)/magnitude;
            float dist = ImSqrt(ImLengthSqr(w) - projection_length*projection_length);
            result = ( dist <= proximity_threshold && projection_length < magnitude);
        }
        return result;
    };
    auto hovering_which_node = [&]()->node{
            for(auto& n:G.nodes()){
                if ( n.second->m_rect.Contains(ImGui::GetMousePos()) ) return n.second;
            }
            return nullptr;
    };
    auto hovering_which_edge = [&]()->edge{
        if(G.edges().size()>0)
        for(auto& e: G.edges()){
            int segments = e.second->m_polyline.size() - 1;
            if( segments>0  ){
                vector<ImVec2> &bends = e.second->m_polyline;
                for (size_t i = 1; i <= segments; i++)
                {
                    if( mouse_proximity_test(bends[i-1], bends[i], proximity_threshold) ) return e.second;
                }
            }
        }
        return nullptr;
    };
    auto hovering_which_cluster = [&]() -> cluster {
        for( auto& c: G.clusters() ){
            if ( c.second->m_rect.Contains(ImGui::GetMousePos()) ) return c.second;
        }
        return nullptr;
    };

    auto focused_node = []()->node{
        return G.choose_focused_node();
    };
    auto focused_edge = []()->edge{
        return G.choose_focused_edge();
    };
    auto focused_cluster = []()->cluster{
        return G.choose_focused_cluster();
    };
    auto update_cluster_bbox = [&](cluster c)->ImRect {
        auto nodes = c->m_child_nodes;
        assert(nodes.size() > 0);
        ImVec2 min(nodes.front()->m_rect.Min);
        ImVec2 max(nodes.front()->m_rect.Max);
        for(auto i:nodes){
            min = ImMin(min, i->m_rect.Min);
            max = ImMax(max, i->m_rect.Max);
        }
        ImRect bbox(min, max);
        c->m_padding = std::max(cluster_padding, ImGui::GetFontSize());
        c->m_margin = cluster_margin;
        c->m_rect = ImRect(min, max);
        c->m_rect.Expand(c->m_padding);
        return c->m_rect;
    };

    auto update_graph_attributes = [&](bool generate_label){
        //update node attributes
        ImGui::PushFont(node_font);
        for(auto& n:G.nodes()){
            string label = generate_label? fmt::format("Node {}", n.second->m_id):n.second->m_label;
            auto tsz = ImGui::CalcTextSize(label.c_str(), NULL, false, 0.0f);
            n.second->m_label = label;
            n.second->m_label_sz = tsz;
            n.second->m_label_margin = node_text_margins;
            n.second->m_rect.Max = n.second->m_rect.Min + ImVec2(  max(n.second->m_image_rect.GetWidth() , tsz.x + 2*node_text_margins), 
                                                        n.second->m_image_rect.GetHeight() + tsz.y + 2*node_text_margins);
            n.second->m_label_font_size = ImGui::GetFontSize();
        }
        ImGui::PopFont();  

        //update edge attributes
        ImGui::PushFont(edge_font);
        for(auto& e:G.edges()){
            ImRect r_src(e.second->m_source->m_rect);
            ImVec2 smin = r_src.Min;
            ImVec2 ssz = r_src.GetSize();
            ImRect r_dst(e.second->m_target->m_rect);
            ImVec2 tmin = r_dst.Min;
            ImVec2 tsz = r_dst.GetSize();
            ImVec2 tail, head, p1, p2, p3, p4;
            vector<ImVec2> bends = e.second->m_bends;//input from layout algorithm
            size_t bend = bends.size();
            if ( bend == 0 )
            {
                if(!contact_point_type)
                {
                    calculate_line_vertex(smin, ssz, tmin, tsz, tail, head);
                }
                else
                {
                    tail = ImRect_ClosestPoint(r_src, r_dst);
                    head = ImRect_ClosestPoint(r_dst, r_src);
                }
                create_triangle_vertex(tail, head, p1, p2, arrow_angle, arrow_head_sz);
                create_triangle_vertex(head, tail, p3, p4, arrow_angle, arrow_head_sz);
            }
            else if ( bend > 0 )
            {
                if(!contact_point_type)
                {
                    calculate_rectangle_contact_point(smin, ssz, bends.front(), tail);
                    calculate_rectangle_contact_point(tmin, tsz, bends.back(), head);
                }
                else
                {
                    tail = ImRect_ClosestPoint(r_src, bends.front(), false);
                    head = ImRect_ClosestPoint(r_dst, bends.back(), false);
                }
                create_triangle_vertex(bends.back(), head, p1, p2, arrow_angle, arrow_head_sz);
                create_triangle_vertex(bends.front(), tail, p3, p4, arrow_angle, arrow_head_sz);
            }
            e.second->m_polyline.clear();
            e.second->m_polyline.push_back(tail);
            if( bend ) e.second->m_polyline.insert(e.second->m_polyline.end(), make_move_iterator(bends.begin()), make_move_iterator(bends.end()));
            e.second->m_polyline.push_back(head);

            string label = generate_label? fmt::format("Edge {}", e.second->m_id):e.second->m_label;
            auto ret = get_point_on_polyline(e.second->m_label_position, e.second->m_polyline);
            ImVec2 cp = get<ImVec2>(ret);
            ImVec2 lsz = ImGui::CalcTextSize(label.c_str(), NULL, false, 0.0f);
            e.second->m_label = label;
            e.second->m_label_rect = ImRect(cp - 0.5f*lsz, cp + 0.5f*lsz);
            e.second->m_total_length = get<float>(ret);
            e.second->m_arrow_ear1 = p1;
            e.second->m_arrow_ear2 = p2;
            e.second->m_arrow_ear3 = p3;
            e.second->m_arrow_ear4 = p4;
            e.second->m_label_font_size = ImGui::GetFontSize();
        }
        ImGui::PopFont();

        //update cluster attributes
        ImGui::PushFont(cluster_font);
        for(auto& c:G.clusters())
        {
            if(  c.second == G.root_cluster() ) continue;//skip root cluster
            ImRect rc = update_cluster_bbox(c.second);
            c.second->m_label_rect = ImRect(rc.Min, rc.Min + ImGui::CalcTextSize(c.second->m_label.c_str(), NULL, false, 0.0f));
            c.second->m_label_font_size = ImGui::GetFontSize();
        }
        ImGui::PopFont();
    };

    // search alignment guidline
    auto search_alignment_guidline = [&](node center, float threshold0, float threshold1){
        alignments.clear();
        map<float, alignment_t> v_alignments;
        map<float, alignment_t> h_alignments;
        vector<ImVec2> c_anchors = {center->m_rect.GetTL(), ImLerp(center->m_rect.GetTL(), center->m_rect.GetTR(), 0.5f), center->m_rect.GetTR(),
                                    center->m_rect.GetTR(), ImLerp(center->m_rect.GetTR(), center->m_rect.GetBR(), 0.5f), center->m_rect.GetBR()};
        for(auto n:G.nodes()){
            if( n.second == center ) continue;
            ImLine d = ImRect_ClosestLine(n.second->m_rect, center->m_rect);
            float distance = ImLength(d.A - d.B);
            if( distance < threshold0 ){
                vector<ImVec2> n_anchors = {n.second->m_rect.GetTL(), ImLerp(n.second->m_rect.GetTL(), n.second->m_rect.GetTR(), 0.5f), n.second->m_rect.GetTR(),
                                            n.second->m_rect.GetTR(), ImLerp(n.second->m_rect.GetTR(), n.second->m_rect.GetBR(), 0.5f), n.second->m_rect.GetBR()};
                bool v_align_valid = false;
                bool h_align_valid = false;
                alignment_t v_align;
                alignment_t h_align;
                for (size_t i = 0; i < c_anchors.size(); i++)
                {
                    for (size_t j = 0; j < n_anchors.size(); j++)
                    {
                        if( i < 3 && j < 3 ){
                            ImVec2 delta = c_anchors[i] - n_anchors[j];
                            if( fabs(delta.x) < threshold1 ){
                                ImVec2 a(n_anchors[j].x, d.A.y), b(n_anchors[j].x, d.B.y);
                                if( ImLength(a - b) > 1e-4f ){
                                    v_align.m_id1 = center->m_id;
                                    v_align.m_id2 = n.second->m_id;
                                    v_align.m_situations.push_back(static_cast<situation_t>(1<<i | 1<<(j+6)));
                                    v_align.guidlines.push_back(make_pair(a, b));
                                    v_align.guidpoints.push_back(ImVec2(c_anchors[i].x, d.B.y));
                                    v_align_valid = true;
                                }
                            }
                        }else if( i >= 3 && j >= 3 ){
                            ImVec2 delta = c_anchors[i] - n_anchors[j];
                            if( fabs(delta.y) < threshold1 ){
                                ImVec2 a(d.A.x, n_anchors[j].y), b(d.B.x, n_anchors[j].y);
                                if( ImLength(a - b) > 1e-4f ){
                                    h_align.m_id1 = center->m_id;
                                    h_align.m_id2 = n.second->m_id;
                                    h_align.m_situations.push_back(static_cast<situation_t>(1<<i | 1<<(j+6)));
                                    h_align.guidlines.push_back(make_pair(a, b));
                                    h_align.guidpoints.push_back(ImVec2(d.B.x, c_anchors[i].y));
                                    h_align_valid = true;
                                }
                            }
                        }
                    }
                }
                if( v_align_valid ) v_alignments[distance] = v_align;
                if( h_align_valid ) h_alignments[distance] = h_align;
            }
        }
        //only keep the nearest alignment
        if( !v_alignments.empty() ){
            auto it = v_alignments.begin();
            alignments.push_back(it->second);
        }
        if( !h_alignments.empty() ){
            auto it = h_alignments.begin(); 
            alignments.push_back(it->second);
        }
    };

    bool is_moving = false;
    bool middle_clicked = ImGui::IsMouseClicked(2);
    bool left_dragging = ImGui::IsMouseDragging(0, 2);
    bool right_dragging = ImGui::IsMouseDragging(1, 2);
    bool ctrl_down = ImGui::IsKeyDown(ImGuiKey_LeftCtrl, false) || ImGui::IsKeyDown(ImGuiKey_RightCtrl, false);
    bool shift_down = ImGui::IsKeyDown(ImGuiKey_LeftShift, false) || ImGui::IsKeyDown(ImGuiKey_RightShift, false);
    bool is_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_DockHierarchy);
    bool is_window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_DockHierarchy);
    bool active_node_label_editor = false, active_edge_label_editor = false, active_group_label_editor = false;
    node node_hovering = hovering_which_node();

    //input
    sm.node_hovering = node_hovering;
    sm.edge_hovering = hovering_which_edge();
    sm.cluster_hovering = hovering_which_cluster();
    sm.is_hovering_pin = [&](node n)->unsigned int{
        if(!n ||  ImLength(n->m_image_rect.GetSize()) <= 0) return 0;
        ImVec2 cp = n->m_rect.GetCenter();
        ImVec2 p1 = ImNormalized(n->m_rect.GetTL() - cp)*pin_radius + n->m_rect.GetTL();
        ImVec2 p2 = ImNormalized(n->m_rect.GetTR() - cp)*pin_radius + n->m_rect.GetTR();
        ImVec2 p3 = ImNormalized(n->m_rect.GetBL() - cp)*pin_radius + n->m_rect.GetBL();
        ImVec2 p4 = ImNormalized(n->m_rect.GetBR() - cp)*pin_radius + n->m_rect.GetBR();
        unsigned int result = 0;
        float sacle = canvas.View().InvScale;
             if( result = ImLength(p1 - io.MousePos) < sacle*proximity_threshold ) { result = 1; }
        else if( result = ImLength(p2 - io.MousePos) < sacle*proximity_threshold ) { result = 2; }
        else if( result = ImLength(p3 - io.MousePos) < sacle*proximity_threshold ) { result = 3; }
        else if( result = ImLength(p4 - io.MousePos) < sacle*proximity_threshold ) { result = 4; }
        return result;
    };
    sm.left_down = ImGui::IsMouseDown(0);
    sm.left_released = ImGui::IsMouseReleased(0);
    sm.left_clicked = ImGui::IsMouseClicked(0);
    sm.left_double_clicked = ImGui::IsMouseDoubleClicked(0);
    sm.left_dragging = left_dragging;
    sm.ctrl_down = ctrl_down;
    sm.shift_down = shift_down;
    sm.alt_down = ImGui::IsKeyPressed(ImGuiKey_LeftAlt, false) || ImGui::IsKeyPressed(ImGuiKey_RightAlt, false);
    sm.tab_down = ImGui::IsKeyPressed(ImGuiKey_Tab, false);
    sm.enter_down = ImGui::IsKeyPressed(ImGuiKey_Enter, false);
    sm.is_window_hovered = is_window_hovered;
    sm.mouse_pos = io.MousePos;
    sm.is_window_focused = is_window_focused;
    sm.want_select_all = ImGui::Shortcut(key_chord_select_all, input_flags);
    sm.want_delete = !want_editing && ImGui::IsKeyDown(ImGuiKey_Delete, false);
    sm.want_copy = ImGui::Shortcut(key_chord_copy, input_flags);
    sm.want_paste = ImGui::Shortcut(key_chord_paste, input_flags);
    sm.want_cut = ImGui::Shortcut(key_chord_cut, input_flags);
    sm.want_undo = ImGui::Shortcut(key_chord_undo, input_flags | ImGuiInputFlags_Repeat);
    sm.want_redo = ImGui::Shortcut(key_chord_redo, input_flags | ImGuiInputFlags_Repeat);
    
    //output
    sm.is_node_focused = [&](node i){ return i? i->m_focused:false; };
    sm.is_edge_focused = [&](edge i){ return i? i->m_focused:false; };
    sm.is_cluster_focused = [&](cluster i){ return i? i->m_focused:false; };
    sm.count_selected_node = []() ->int { return G.count_focused_node(); };
    sm.count_selected_edge = []() ->int { return G.count_focused_edge(); };
    sm.count_selected_cluster = []() ->int { return G.count_focused_cluster(); };
    sm.get_selected_node = []()->node{ return G.choose_focused_node(); };
    sm.get_selected_nodes = []()->vector<node>{ return G.choose_focused_nodes(); };
    sm.create_node = [&](void)->node {
        node ret = nullptr;
        auto cmd = make_unique<CreateNodeCommand>(G, io.MousePos,[&](node n){
            ImGui::PushFont(node_font);
            string label = fmt::format("N{}", n->m_id);
            auto tsz = ImGui::CalcTextSize(label.c_str(), NULL, false, 0.0f);
            ImGui::PopFont();
            ImVec2 cp(io.MousePos);
            n->m_label = label;
            n->m_rect = ImRect(cp-0.5f*tsz, cp+0.5f*tsz);
            n->m_rect.Expand(node_text_margins);
            n->m_bd_col = node_border_col;
            n->m_bg_col = node_fill_col;
            n->m_txt_col = node_text_col;
            n->m_image = 0;
            n->m_image_rect.Max = n->m_image_rect.Min = {};
            ret = n;
            active_node_label_editor = (auto_active_label_editor > 1);
        });
        GE.execute_command(std::move(cmd));
        return ret;
    };
    sm.create_child = [&](node parent) ->node {
        node n = sm.create_node();
        vector<node> src(1,parent);
        sm.create_edge(src, n);
        return n;
    };
    sm.create_sibling = [&](node sibling) ->node {
        node n = sm.create_node();
        edge e = G.choose_edge([sibling](edge e) { return e->m_target->m_id == sibling->m_id; });
        if(e){
            vector<node> src(1,e->m_source);
            sm.create_edge(src, n);
        }
        return n;
    };
    sm.create_edge = [&](vector<node> src, node dst)->vector<edge>{
        vector<edge> edges;
        for(auto& n: src)
        {
            auto cmd = make_unique<CreateEdgeCommand>(G, n, dst, [&](edge e){
                e->m_label = fmt::format("E{}", e->m_id);
                e->m_arrow = arrow_e::normal;
                e->m_ideal_length = 350.0f;
                e->m_edge_col = edge_col;
                e->m_txt_col = node_text_col;
                edges.push_back(e);
                active_edge_label_editor = (auto_active_label_editor > 1);
            });
            GE.execute_command(std::move(cmd));
        }
        return edges;
    };
    sm.create_cluster = [&](vector<node> nodes)->cluster{
            cluster ret = nullptr;
            auto cmd = make_unique<CreateClusterCommand>(G, nodes, [&](cluster c){
                c->m_label = fmt::format("C{}", c->m_id);
                c->m_bd_col = node_border_col;
                c->m_bg_col = cluster_background_col;
                c->m_txt_col = cluster_text_col;
                update_cluster_bbox(c);
                ret = c;
                active_group_label_editor = (auto_active_label_editor > 1);
            });
            GE.execute_command(std::move(cmd));
            return ret;
    };
    sm.select_all_node = [](){ G.select_all_node(); };
    sm.select_all_edge = [](){ G.select_all_edge(); };
    sm.select_all_cluster =[](){ G.select_all_cluster(); };
    sm.toggle_select_node = [](node n){ G.toggle_select_node(n); };
    sm.toggle_select_edge = [](edge e){ G.toggle_select_edge(e); };
    sm.toggle_select_cluster = [](cluster c){ G.toggle_select_cluster(c); };
    sm.single_select_node = [](node n){ G.select_single_node(n); };
    sm.single_select_edge = [](edge e){ G.select_single_edge(e);};
    sm.single_select_cluster = [&](cluster c){ G.select_single_cluster(c);};
    sm.aera_select_object = [&](ImVec2 start, ImVec2 end){
        //draw a rectangle selection aera
        ImVec2 min(std::min(start.x, end.x), std::min(start.y, end.y));
        ImVec2 max(std::max(start.x, end.x), std::max(start.y, end.y));
        ImRect select_aera(min, max);
        splitter.SetCurrentChannel(drawlist, 0);
        drawlist->AddRectFilled(min, max, select_box_fill_col);

        sm.unselect_all_node();
        sm.unselect_all_edge();
        sm.unselect_all_cluster();
        //node
        for(auto n:G.nodes()){
            if( select_aera.Contains(n.second->m_rect) ) sm.toggle_select_node(n.second);
        }
        //edge?
        //cluster?
    };
    sm.unselect_all_node = [](){ G.unselect_all_node(); };
    sm.unselect_all_edge = [](){ G.unselect_all_edge();};
    sm.unselect_all_cluster = [](){G.unselect_all_cluster();};
    sm.lock_focused_node = [](){ G.lock_focused_node(); };
    sm.unlock_focused_node = [](){ G.unlock_focused_node();};
    sm.move_node_begin = [&](){
        is_moving = true;
        should_update_graph_attributes  = true;
        int focused_count = G.count_focused_node();
        auto delta = ImGui::GetIO().MouseDelta / canvas.ViewScale();
        //moving selected node
        for(auto& n:G.nodes()){
            if ( n.second->m_focused ){
                n.second->m_rect.Translate(delta);
            }
        }
    };
    sm.move_node_end = [&](ImVec2 from, ImVec2 to){
        GE.execute_command(make_unique<MovingNodeCommand>(G, G.choose_focused_nodes(), from, to));
    };
    sm.resize_image_begin = [&](unsigned int pin){
        ImVec2 pin_trans[5] = {ImVec2(0.0f,0.0f), ImVec2(-1.0f,-1.0f), ImVec2(1.0f,-1.0f), ImVec2(-1.0f,1.0f), ImVec2(1.0f,1.0f)};
        should_update_graph_attributes  = true;
        node n = G.choose_focused_node();
        if(n && ImLength(n->m_image_rect.GetSize()) > 0){
            auto delta = pin_trans[pin]*ImGui::GetIO().MouseDelta / canvas.ViewScale();
            n->m_image_rect.Max += get_projection( n->m_image_rect.GetSize(), delta);
        }
    };
    sm.resize_image_end = [&](ImVec2 start, ImVec2 end, unsigned int pin){
        node n = G.choose_focused_node();
        if(n && ImLength(n->m_image_rect.GetSize()) > 0){
            ImVec2 pin_trans[5] = {ImVec2(0.0f,0.0f), ImVec2(-1.0f,-1.0f), ImVec2(1.0f,-1.0f), ImVec2(-1.0f,1.0f), ImVec2(1.0f,1.0f)};
            auto delta = get_projection(n->m_image_rect.GetSize(), pin_trans[pin]*(end - start));
            GE.execute_command(make_unique<ImageSZ>(G, n, delta));
        }
    };
    sm.search_guidline = [&](){
        if ( enable_continue_layout || G.count_focused_node() != 1 ) return;
        node n = G.choose_focused_node();
        search_alignment_guidline(n, guideline_search_threshold, guideline_snap_threshold);
        //draw alignment guidline
        for(auto a:alignments){
            for(auto& i:a.guidlines){
                splitter.SetCurrentChannel(drawlist, 4);
                drawlist->AddLine(i.first, i.second, node_border_col);
            }
            for(auto i: a.guidpoints){
                splitter.SetCurrentChannel(drawlist, 4);
                drawlist->AddCircleFilled(i, 5, node_border_col);
            }
        }
    };
    sm.apply_guidline = [&](){
        if ( enable_continue_layout || alignments.empty() ) return;
        // apply alignment
        for(auto a:alignments){
            node n1 = G.get_node(a.m_id1);
            node n2 = G.get_node(a.m_id2);
            if( !n1 || !n2 ) continue;//this is bad.
            ImVec2 v1_c =ImLerp(n1->m_rect.GetTL(), n1->m_rect.GetTR(), 0.5f);
            ImVec2 h1_c =ImLerp(n1->m_rect.GetTR(), n1->m_rect.GetBR(), 0.5f);
            ImVec2 v2_c =ImLerp(n2->m_rect.GetTL(), n2->m_rect.GetTR(), 0.5f);
            ImVec2 h2_c =ImLerp(n2->m_rect.GetTR(), n2->m_rect.GetBR(), 0.5f);
            ImVec2 delta(0.0f, 0.0f);
            switch ((a.m_situations.size() == 3)? a.m_situations[1]:a.m_situations[0])
            {
                case situation_t::v_align_ll:
                delta = n1->m_rect.GetTL() - n2->m_rect.GetTL();
                delta.y = 0;
                break;
                case situation_t::v_align_lc:
                delta = n1->m_rect.GetTL() - v2_c;
                delta.y = 0;
                break;
                case situation_t::v_align_lr:
                delta = n1->m_rect.GetTL() - n2->m_rect.GetTR();
                delta.y = 0;
                break;
                case situation_t::v_align_cl:
                delta = v1_c - n2->m_rect.GetTL();
                delta.y = 0;
                break;
                case situation_t::v_align_cc:
                delta = v1_c - v2_c;
                delta.y = 0;
                break;
                case situation_t::v_align_cr:
                delta = v1_c - n2->m_rect.GetTR();
                delta.y = 0;
                break;
                case situation_t::v_align_rl:
                delta = n1->m_rect.GetTR() - n2->m_rect.GetTL();
                delta.y = 0;
                break;
                case situation_t::v_align_rc:
                delta = n1->m_rect.GetTR() - v2_c;
                delta.y = 0;
                break;
                case situation_t::v_align_rr:
                delta = n1->m_rect.GetTR() - n2->m_rect.GetTR();
                delta.y = 0;
                break;
                case situation_t::h_align_tt:
                delta = n1->m_rect.GetTR() - n2->m_rect.GetTR();
                delta.x = 0;
                break;
                case situation_t::h_align_tc:
                delta = n1->m_rect.GetTR() - h2_c;
                delta.x = 0;
                break;
                case situation_t::h_align_tb:
                delta = n1->m_rect.GetTR() - n2->m_rect.GetBR();
                delta.x = 0;
                break;
                case situation_t::h_align_ct:
                delta = h1_c - n2->m_rect.GetTR();
                delta.x = 0;
                break;
                case situation_t::h_align_cc:
                delta = h1_c - h2_c;
                delta.x = 0;
                break;
                case situation_t::h_align_cb:
                delta = h1_c - n2->m_rect.GetBR();
                delta.x = 0;
                break;
                case situation_t::h_align_bt:
                delta = n1->m_rect.GetBR() - n2->m_rect.GetTR();
                delta.x = 0;
                break;
                case situation_t::h_align_bc:
                delta = n1->m_rect.GetBR() - h2_c;
                delta.x = 0;
                break;
                case situation_t::h_align_bb:
                delta = n1->m_rect.GetBR() - n2->m_rect.GetBR();
                delta.x = 0;
                break;
            default:
                break;
            }
            n1->m_rect.Translate(-delta);
            should_update_graph_attributes  = true;
        }
    };
    sm.move_cluster_begin = [&](){
        is_moving = true; 
        should_update_graph_attributes  = true;
        auto delta = ImGui::GetIO().MouseDelta / canvas.ViewScale();
        for( auto& c: G.clusters() ){
            if( c.second->m_focused ){
                for( auto& n: c.second->m_child_nodes ){
                    sm.toggle_select_node(n);
                    n->m_rect.Translate(delta);
                }
                G.lock_focused_node();
            }
        }
    };
    sm.move_cluster_end = [&](ImVec2 from, ImVec2 to){
        GE.execute_command(make_unique<MovingClusterCommand>(G, G.choose_focused_clusters(), from, to));
    };
    sm.node_properties_popup = [&](node n){active_node_label_editor = (auto_active_label_editor == 1 || auto_active_label_editor == 3);};
    sm.edge_properties_popup = [&](edge e){active_edge_label_editor = (auto_active_label_editor == 1 || auto_active_label_editor == 3);};
    sm.cluster_properties_popup = [&](cluster c){active_group_label_editor = (auto_active_label_editor == 1 || auto_active_label_editor == 3);};
    sm.draw_links = [&](){
        ImGui::PushFont(mdicon);
        for(auto i:G.nodes()){
            if( i.second->m_focused ){
                ImVec2 p1;
                calculate_rectangle_contact_point(i.second->m_rect.Min, i.second->m_rect.GetSize(), ImGui::GetMousePos(), p1);
                splitter.SetCurrentChannel(drawlist, 0);
                drawlist->AddCircleFilled(p1, dot_sz, linking_col);
                splitter.SetCurrentChannel(drawlist, 0);
                drawlist->AddLine(p1, ImGui::GetMousePos(), linking_col);
                splitter.SetCurrentChannel(drawlist, 0);
                drawlist->AddText(ImLerp(p1, ImGui::GetMousePos(), 0.5f), linking_col,"" ICON_MD_ADD_LINK);
            }
        }
        ImGui::PopFont();

        if(node_hovering)
        {
            ImRect rect = node_hovering->m_rect;
            float space(border_thickness * canvas.View().InvScale);
            rect.Expand(space);
            splitter.SetCurrentChannel(drawlist, 3);
            drawlist->AddRect(rect.Min, rect.Max, linking_col, 5.0f, 0, space);
        }
    };
    sm.is_clipbord_data_valid = [&]()->bool{
        using json = nlohmann::json;
        bool result = false;
        json j;
        try{
            j = json::parse(ImGui::GetClipboardText());
            result = j.contains("nodes") && j.contains("edges");
        }catch(...){
            result = false;
        }
        return result;
    };
    sm.delete_selected = [&](){ GE.execute_command(make_unique<DeleteSelectedCommand>(G)); };
    sm.copy_selected= [&](){ GE.execute_command(make_unique<CopyToClipboardCommand>(G)); };
    sm.paste_selected= [&](){ GE.execute_command(make_unique<PasteFromClipboardCommand>(G, io.MousePos)); };
    sm.cut_selected= [&](){};
    sm.save_graph = [&](){ };
    sm.undo_command = [&](){ GE.undo();};
    sm.redo_command= [&](){ GE.redo(); };
    sm.loop();
    G.loop();
    
    if(show_debug_window){
        sm.debug();
        G.debug();
        GE.debug();
    }

    //===============================================Edit logic end====================================================

    //===============================================Canvas logic start================================================
    static bool is_canvas_changed = false;
    if ( is_window_hovered && right_dragging )
    {
        origin =  canvas.ViewOrigin() + ImGui::GetIO().MouseDelta;
        is_canvas_changed = true;
    }
    if( is_window_hovered && ImGui::GetIO().MouseWheel )
    {
        scale += 0.1f*ImGui::GetIO().MouseWheel;
        scale = ImClamp(scale, 0.1f, 2.0f);
        float delta_scale = scale - canvas.ViewScale();
        origin = canvas.ViewOrigin() - ImGui::GetIO().MousePos*delta_scale;
        is_canvas_changed = true;
    }
    if( is_window_hovered && middle_clicked )
    {
        scale = 1.0f;
        float delta_scale = scale - canvas.ViewScale();
        origin = canvas.ViewOrigin() - ImGui::GetIO().MousePos*delta_scale;
        is_canvas_changed = true;
    }
    if ( is_canvas_changed )
    {
        ImGuiEx::CanvasView view(origin, scale);
        canvas.SetView(view);
        // GE.execute_command(make_unique<CanvasCommand>(canvas, view));
        is_canvas_changed = false;
    }

    //panning animation
    float s = ImLength(canvas_panning_offset);
    if( s>1 )
    {
        ImGuiEx::CanvasView view(canvas.View());
        ImVec2 delta = max(100.0f, canvas_panning_speed*s/canvas_panning_distance) * ImNormalized(canvas_panning_offset)*ImGui::GetIO().DeltaTime;
        delta = (s > ImLength(delta)?delta:canvas_panning_offset);
        view.Origin += delta;
        canvas.SetView(view);
        canvas_panning_offset -= delta;
    }
    //===============================================Canvas logic end==================================================

    //==================================================Draw Graph Begin================================================
    //prepare for drawing
    static int draw_check = 0;
    should_update_graph_attributes |= G.changed(draw_check);
    if( should_update_graph_attributes ){
        update_graph_attributes(false);
        should_update_graph_attributes = false;
    }
    auto draw_bbox = [&](ImRect bbox){
        float space(border_thickness * canvas.View().InvScale);
        splitter.SetCurrentChannel(drawlist, 3);
        bbox.Expand(0.5f*space);
        drawlist->AddRect(bbox.Min, bbox.Max, focused_col, 5.0f, 0, space);
    };
    //draw clusters , nodes and edges
    ImGui::PushFont(cluster_font);
    for(auto c:G.clusters())
    {
        if(  c.second == G.root_cluster() ) continue;//skip root cluster
        ImRect rc = c.second->m_rect;
        splitter.SetCurrentChannel(drawlist, 0);
        drawlist->AddRectFilled(rc.Min, rc.Max, c.second->m_bg_col, 5, ImDrawFlags_RoundCornersAll);//background
        splitter.SetCurrentChannel(drawlist, 0);
        drawlist->AddText(rc.GetTL(), c.second->m_txt_col, c.second->m_label.c_str());//label
        splitter.SetCurrentChannel(drawlist, 0);
        rc.Expand(0.5f*border_thickness);
        drawlist->AddRect(rc.Min, rc.Max, c.second->m_bd_col, 5, ImDrawFlags_RoundCornersAll);//border
        if( c.second->m_focused ) draw_bbox(rc);
    }
    ImGui::PopFont();
    ImGui::PushFont(node_font);
    for(auto n:G.nodes())
    {
        ImDrawFlags flags = ImDrawFlags_RoundCornersAll;
        ImRect rect = n.second->m_rect;
        ImRect bbox = rect;
        bbox.Expand(0.5*border_thickness);
        bool focused = n.second->m_focused;
        int level = focused?3:2;
        splitter.SetCurrentChannel(drawlist, level);
        drawlist->AddRect(bbox.Min, bbox.Max, n.second->m_bd_col, 5, flags, border_thickness);//border
        splitter.SetCurrentChannel(drawlist, level);
        drawlist->AddRectFilled(rect.Min, rect.Max, IM_COL32_BLACK, 5, flags);//background mask
        splitter.SetCurrentChannel(drawlist, level);
        drawlist->AddRectFilled(rect.Min, rect.Max, n.second->m_bg_col, 5, flags);//background
        
        //draw label
        if(n.second->m_label.size()) {
            splitter.SetCurrentChannel(drawlist, level);
            ImRect label_rect(rect.Min + ImVec2(0, n.second->m_image_rect.GetHeight()), rect.Max);
            drawlist->AddText(label_rect.GetCenter() - 0.5f*n.second->m_label_sz, n.second->m_txt_col, n.second->m_label.c_str());//label
        }

        //draw image
        if( n.second->m_image ) {
            splitter.SetCurrentChannel(drawlist, level);
            ImVec2 min = rect.GetTL();
            ImVec2 max = rect.GetTL() + n.second->m_image_rect.GetSize();
            drawlist->AddImage(n.second->m_image, min, max);
        }

        //draw pins
        if( n.second->m_focused && ImLength(n.second->m_image_rect.GetSize())>0){
            ImVec2 cp = n.second->m_rect.GetCenter();
            ImVec2 p1 = ImNormalized(n.second->m_rect.GetTL() - cp)*pin_radius + n.second->m_rect.GetTL();
            ImVec2 p2 = ImNormalized(n.second->m_rect.GetTR() - cp)*pin_radius + n.second->m_rect.GetTR();
            ImVec2 p3 = ImNormalized(n.second->m_rect.GetBL() - cp)*pin_radius + n.second->m_rect.GetBL();
            ImVec2 p4 = ImNormalized(n.second->m_rect.GetBR() - cp)*pin_radius + n.second->m_rect.GetBR();
            splitter.SetCurrentChannel(drawlist, level);
            drawlist->AddCircleFilled(p1, pin_radius*canvas.View().InvScale, focused_col);
            splitter.SetCurrentChannel(drawlist, level);
            drawlist->AddCircleFilled(p2, pin_radius*canvas.View().InvScale, focused_col);
            splitter.SetCurrentChannel(drawlist, level);
            drawlist->AddCircleFilled(p3, pin_radius*canvas.View().InvScale, focused_col);
            splitter.SetCurrentChannel(drawlist, level);
            drawlist->AddCircleFilled(p4, pin_radius*canvas.View().InvScale, focused_col);
        }

        //draw icon
        if ( n.second->m_locked ) {
            ImGui::PushFont(mdicon);
            splitter.SetCurrentChannel(drawlist, level);
            drawlist->AddText(rect.GetTR(), lock_icon_col, ICON_MD_LOCK_OUTLINE);//lock mark
            ImGui::PopFont();
        }

        //draw bounding box
        if( focused ) {
            draw_bbox(bbox);
        }
    }
    ImGui::PopFont();

    //draw edges
    ImGui::PushFont(edge_font);
    for(auto& e:G.edges())
    {   bool edge_selected = e.second->m_focused;
        bool source_selected = e.second->m_source->m_focused;

        //edge polyline
        float polyline_thickness = arrow_line_thickness*((edge_selected || source_selected)?2.0f:1.0f);
        float arrow_ear_thickness = arrow_head_thickness*((edge_selected || source_selected)?2.0f:1.0f);
        splitter.SetCurrentChannel(drawlist, 0);
        drawlist->AddPolyline(e.second->m_polyline.data(), e.second->m_polyline.size(), edge_selected?ImU32(focused_col):e.second->m_edge_col, 0, polyline_thickness);

        //arrow's ear
        if( e.second->m_arrow == mindmap::arrow_t::normal ){
            splitter.SetCurrentChannel(drawlist, 0);
            drawlist->AddLine(e.second->m_arrow_ear1, e.second->m_polyline.back(), edge_selected?ImU32(focused_col):e.second->m_edge_col, arrow_ear_thickness);
            splitter.SetCurrentChannel(drawlist, 0);
            drawlist->AddLine(e.second->m_arrow_ear2, e.second->m_polyline.back(), edge_selected?ImU32(focused_col):e.second->m_edge_col, arrow_ear_thickness);
        }
        else if( e.second->m_arrow == mindmap::arrow_t::dual ){
            splitter.SetCurrentChannel(drawlist, 0);
            drawlist->AddLine(e.second->m_arrow_ear1, e.second->m_polyline.back(), edge_selected?ImU32(focused_col):e.second->m_edge_col, arrow_ear_thickness);
            splitter.SetCurrentChannel(drawlist, 0);
            drawlist->AddLine(e.second->m_arrow_ear2, e.second->m_polyline.back(), edge_selected?ImU32(focused_col):e.second->m_edge_col, arrow_ear_thickness);
            splitter.SetCurrentChannel(drawlist, 0);
            drawlist->AddLine(e.second->m_arrow_ear3, e.second->m_polyline.front(), edge_selected?ImU32(focused_col):e.second->m_edge_col, arrow_ear_thickness);
            splitter.SetCurrentChannel(drawlist, 0);
            drawlist->AddLine(e.second->m_arrow_ear4, e.second->m_polyline.front(), edge_selected?ImU32(focused_col):e.second->m_edge_col, arrow_ear_thickness);
        }

        //edge label
        if( e.second->m_label.size() ){
            splitter.SetCurrentChannel(drawlist, 1);
            drawlist->AddRectFilled(e.second->m_label_rect.Min, e.second->m_label_rect.Max, ImColor(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]), 10);
            splitter.SetCurrentChannel(drawlist, 1);
            drawlist->AddText(e.second->m_label_rect.Min, e.second->m_txt_col, e.second->m_label.c_str());
        }

        //flow animation
        bool should_flow = (flow_speed > 0 && e.second->m_total_length > 1);
        should_flow &= ( flow_direction? e.second->m_source->m_focused:e.second->m_target->m_focused );
        if(should_flow){
            e.second->m_flow_position += ImGui::GetIO().DeltaTime * flow_speed;
            int n_dot = std::max(1.0f, roundf(e.second->m_total_length / ideal_dot_gap));
            float gap = e.second->m_total_length/n_dot;
            for (size_t i = 0; i <= n_dot; i++){
                float p = fmodf(e.second->m_flow_position - i*gap, e.second->m_total_length);
                p = clamp(p/e.second->m_total_length, 0.0f, 1.0f);
                auto x = get_point_on_polyline(p, e.second->m_polyline);
                splitter.SetCurrentChannel(drawlist, 0);
                drawlist->AddCircleFilled(get<ImVec2>(x), dot_sz, edge_selected?ImU32(focused_col):e.second->m_edge_col);
            }
        }
    }

    ImGui::PopFont();
    splitter.Merge(drawlist);

    if( canvas_enabled ) canvas.End();
    ImGui::End();
    //==================================================Draw Graph End==================================================


    //==================================================Edge List Begin=================================================
    ImGui::Begin("Edges List");
    bool lengths_changed = false;
    bool label_positions_changed = false;
    ImGuiTableColumnFlags table_flags = ImGuiTableColumnFlags_None 
                | ImGuiTableFlags_Borders| ImGuiTableFlags_BordersH| ImGuiTableFlags_BordersOuterH
                | ImGuiTableFlags_BordersInnerH| ImGuiTableFlags_BordersV| ImGuiTableFlags_BordersOuterV
                | ImGuiTableFlags_BordersInnerV| ImGuiTableFlags_BordersOuter| ImGuiTableFlags_BordersInner 
                | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_RowBg;
    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
    if (ImGui::BeginTable("table_edges", 4, table_flags|ImGuiTableFlags_PadOuterX, ImVec2(0, 0), 0))
    {
        // Declare columns
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Label position");
        ImGui::TableSetupColumn("Length");
        ImGui::TableSetupColumn("Arrow");
        ImGui::TableHeadersRow();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        for(auto& e:G.edges())
        {
            int i = e.first;
            ImGui::TableNextRow(ImGuiTableRowFlags_None);
            bool item_is_selected = e.second->m_focused;
            if (  is_window_hovered && item_is_selected ){ImGui::SetScrollHereY(0.5f);}
            ImGui::AlignTextToFramePadding();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(fmt::format("{}", i).c_str(), item_is_selected, selectable_flags, ImVec2(0, 0)))
            {
                sm.unselect_all_node();
                sm.unselect_all_cluster();
                sm.single_select_edge(e.second);
                sm.table_selected = true;
                ImVec2 cp = get<ImVec2>(get_point_on_polyline(e.second->m_label_position, e.second->m_polyline));
                auto rect = canvas.ViewRect();
                ImVec2 offset = rect.GetCenter() - cp;
                canvas_panning_offset = offset*scale;
                canvas_panning_distance = ImLength(canvas_panning_offset);
                ImGuiEx::CanvasView view = canvas.View();
                view.Origin += canvas_panning_offset;
                GE.execute_command(make_unique<CanvasCommand>(canvas, canvas.View(), view));
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) )
                ImGui::SetTooltip("[from node:%d to node:%d]", e.second->m_source->m_seq, e.second->m_target->m_seq);
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("reverse")){G.reverse_edge(e.second);};
                if (ImGui::MenuItem("reset")){G.clear_bendpoint(e.second);};
                ImGui::EndPopup();
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            label_positions_changed |= ImGui::DragFloat(fmt::format("##df1{}", i).c_str(), &e.second->m_label_position, 0.002f, 0.001f, 1.0f );
            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            lengths_changed |= ImGui::DragFloat(fmt::format("##df2{}", i).c_str(), &e.second->m_ideal_length, 0.1f, 0.0f, FLT_MAX );
            ImGui::TableSetColumnIndex(3);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            should_update_graph_attributes |= ImGui::Combo(fmt::format("##cb_arrow_type{}", i).c_str(), (int*)(&e.second->m_arrow), "normal\0dual\0none\0");
        }
        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
    ImGui::End();
    //==================================================Edge List End==================================================
    //==================================================Cluster List Begin=================================================
    ImGui::Begin("Cluster List");
    if (ImGui::BeginTable("table_clusters", 2, table_flags|ImGuiTableFlags_PadOuterX, ImVec2(0, 0), 0))
    {
        // Declare columns
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Label");
        ImGui::TableHeadersRow();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        for(auto& c:G.clusters())
        {
            if( c.second == G.root_cluster() ) continue;//skip root cluster
            int i = c.first;
            ImGui::TableNextRow(ImGuiTableRowFlags_None);
            bool item_is_selected = c.second->m_focused;
            if (  is_window_hovered && item_is_selected ){ImGui::SetScrollHereY(0.5f);}
            ImGui::AlignTextToFramePadding();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(fmt::format("{}", i).c_str(), item_is_selected, selectable_flags, ImVec2(0, 0)))
            {
                sm.unselect_all_node();
                sm.unselect_all_edge();
                sm.single_select_cluster(c.second);
                sm.table_selected = true;
                ImVec2 cp = c.second->m_rect.GetCenter();
                auto rect = canvas.ViewRect();
                ImVec2 offset = rect.GetCenter() - cp;
                canvas_panning_offset = offset*scale;
                canvas_panning_distance = ImLength(canvas_panning_offset);
                ImGuiEx::CanvasView view = canvas.View();
                view.Origin += canvas_panning_offset;
                GE.execute_command(make_unique<CanvasCommand>(canvas, canvas.View(), view));
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) )
                ImGui::SetTooltip("%s", c.second->m_label.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText(fmt::format("##input_cluster_label{}", i).c_str(), &c.second->m_label);
        }
        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
    ImGui::End();
    //==================================================Cluster List End==================================================
    //==================================================Node List Begin=================================================
    ImGui::Begin(fmt::format("Node List").c_str());
    static vector<item_t> items;
    static int node_list_check = 0;
    if ( G.changed(node_list_check) )
    {
        items.clear();
        for(auto n: G.nodes()){
            items.push_back(item_s(n.second->m_id,n.second->m_label,n.second->m_indeg,n.second->m_outdeg,n.second->m_locked,string("none")));
        }
    }

    if (ImGui::BeginTable("table_nodes", 6, table_flags, ImVec2(0, 0), 0))
    {
        // Declare columns
        ImGui::TableSetupColumn("ID",           ImGuiTableColumnFlags_DefaultSort );
        ImGui::TableSetupColumn("In",     ImGuiTableColumnFlags_DefaultSort );
        ImGui::TableSetupColumn("Out",    ImGuiTableColumnFlags_DefaultSort );
        ImGui::TableSetupColumn("All",    ImGuiTableColumnFlags_DefaultSort );
        ImGui::TableSetupColumn("Lock",         ImGuiTableColumnFlags_DefaultSort );
        ImGui::TableSetupColumn("Status",       ImGuiTableColumnFlags_DefaultSort );
        // ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
        ImGui::TableHeadersRow();

        // Sort our data if sort specs have been changed!
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
            if (sort_specs->SpecsDirty)
            {
                bool ascending = (sort_specs->Specs->SortDirection > ImGuiSortDirection_Ascending);
                std::sort(items.begin(), items.end(),[&](item_t a, item_t b){
                    bool ret = false;
                    switch ( sort_specs->Specs->ColumnIndex )
                    {
                    case 0:ret =  ascending?(a.m_id > b.m_id):(a.m_id < b.m_id);break;
                    case 1:ret =  ascending?(a.m_indeg > b.m_indeg):(a.m_indeg < b.m_indeg);break;
                    case 2:ret =  ascending?(a.m_outdeg > b.m_outdeg):(a.m_outdeg < b.m_outdeg);break;
                    case 3:ret =  ascending?(a.m_deg > b.m_deg):(a.m_deg < b.m_deg);break;
                    case 4:ret =  ascending?(a.m_locked > b.m_locked):(a.m_locked < b.m_locked);break;
                    case 5:ret = ascending?(a.m_status > b.m_status):(a.m_status < b.m_status);break;
                    default:
                        break;
                    }
                    return ret;
                    });
                sort_specs->SpecsDirty = false;
            }
        ImGuiListClipper clipper;
        clipper.Begin(items.size());
        if( auto ns = sm.get_selected_nodes(); is_window_hovered && ns.size() ) {
            for(auto n:ns) {
                auto it = find_if(items.begin(), items.end(), [n](item_t i){return i.m_id == n->m_id;});
                if(it!=items.end()){
                    int index = std::distance(items.begin(), it);
                    clipper.IncludeItemByIndex(index);
                }
            }
        }
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                string checkbox_id = fmt::format("##cb{}", items[i].m_id);
                ImGui::TableNextRow(ImGuiTableRowFlags_None);
                ImGui::AlignTextToFramePadding();
                ImGui::TableSetColumnIndex(0);

                node v = G.choose_node([&](node n){ return n->m_id == items[i].m_id; });
                // assert(v != nullptr);
                if( v == nullptr ) continue;
                bool item_is_selected = v->m_focused;
                if (  is_window_hovered && item_is_selected && sm.count_selected_node() == 1 )
                {
                    ImGui::SetScrollHereY(0.5f);
                }
                
                string label = fmt::format("{}", items[i].m_id);
                items[i].m_locked = v->m_locked;
                items[i].m_status = fmt::format("{}", item_is_selected?"select":"none");
                
                if (ImGui::Selectable(label.c_str(), item_is_selected, selectable_flags, ImVec2(0, 0)))
                {
                    static int last_selected = -1;
                    if ( shift_down )
                    {
                        if(last_selected >= 0)
                        {
                            int current_selected = min(last_selected, (int)i);
                            sm.unselect_all_node();
                            for (size_t j = current_selected; j <= max(last_selected, (int)i); j++)
                            {
                                node v1 = G.choose_node([&](node n){ return n->m_id == items[j].m_id; });
                                sm.toggle_select_node(v1);
                            }
                        }
                    }
                    else if (ctrl_down)
                    {
                        sm.toggle_select_node(v);
                    }
                    else
                    {
                        last_selected = i;
                        sm.unselect_all_edge();
                        sm.unselect_all_cluster();
                        sm.single_select_node(v);
                        sm.table_selected = true;
                        auto rect = canvas.ViewRect();
                        ImVec2 offset = rect.GetCenter() - v->m_rect.GetCenter();
                        canvas_panning_offset = offset*scale;
                        canvas_panning_distance = ImLength(canvas_panning_offset);
                        ImGuiEx::CanvasView view = canvas.View();
                        view.Origin += canvas_panning_offset;
                        GE.execute_command(make_unique<CanvasCommand>(canvas, canvas.View(), view));
                    }
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ){//show tooltip when hover. show label of current node and all it's children as a markdown list.
                    if(ImGui::BeginTooltip()){
                        ImGui::MarkdownConfig mdConfig{ NULL, NULL, NULL, ICON_FA_LINK, { { H1, true }, { H2, true }, { H3, false } }, NULL }; 
                        string content = v->m_label;
                        util::replace_all(content, "\n", " ");  
                        content.append(": \r\n");
                        content.append("___\n");
                        auto children = G.children(v);
                        for(auto c: children){
                            string item = c->m_label;
                            util::replace_all(item, "\n", " ");
                            content.append(fmt::format("  * {}\r\n", item));
                        }
                        ImGui::Dummy(ImVec2(ImGui::CalcTextSize(content.c_str()).x , 0.0f));  
                        ImGui::Markdown(content.c_str(), content.length(), mdConfig);
                        ImGui::EndTooltip();
                    }
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", items[i].m_indeg);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", items[i].m_outdeg);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", items[i].m_deg);
                ImGui::TableSetColumnIndex(4);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                if(ImGui::Checkbox(checkbox_id.c_str(), &v->m_locked)) {
                    items[i].m_locked = v->m_locked;
                }
                ImGui::PopStyleVar();
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%s", items[i].m_status.c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::End(); 
    //==================================================Node List End===================================================


    //==================================================Edit Properties Begin=================================================
    bool node_label_changed = false;
    bool edge_label_changed = false;
    bool cluster_label_changed = false;
    static std::future<ollama_return_t> ollama_result = {};
    static std::string ollama_url = "http://localhost:11434";
    static std::string ollama_model = "";
    static std::unordered_map<std::string, std::shared_ptr<OllamaServer>> server_list{{ollama_url, std::make_shared<OllamaServer>(ollama_url)}};//<url,server>
    if( server_list.find(ollama_url) != server_list.end() ) {
        if( server_list[ollama_url]->ready()){
            if( ollama_model.empty() )
                ollama_model = server_list[ollama_url]->data().size()?server_list[ollama_url]->data()[0]:"";
        }
    }
    static bool disable_continuously_generate = true;

    string window_label = fmt::format("Edit Properties");

    ImGui::Begin(window_label.c_str());

    // we can eidt one object or one type of object at a time.
    // if we select none object or multiple objects with different types, nothing can be edited. instead, show some tips.
    int selected_node_count = sm.count_selected_node();
    int selected_edge_count = sm.count_selected_edge();
    int selected_cluster_count = sm.count_selected_cluster();
    int selected_count = selected_node_count + selected_edge_count + selected_cluster_count;
    bool only_one_object_selected = selected_count == 1;
    bool different_type_selected = selected_node_count && selected_edge_count || selected_node_count && selected_cluster_count || selected_edge_count && selected_cluster_count;

    ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
    if( ImGui::CollapsingHeader("Tools"))
    {
        ImGui::PushFont(ImGui::GetDefaultFont());
        vector<pair<string, string>> buttons = {
            {ICON_MD_FILE_COPY, "copy."},
            {ICON_MD_CONTENT_PASTE, "paste."},
            {ICON_MD_DELETE, "delete."},
            {ICON_MD_LOCK_OUTLINE, "lock."},
            {ICON_MD_LOCK_OPEN, "unlock."},
            {ICON_MD_IMAGE, "image."},
            {ICON_FA_WAND_MAGIC, "ai."},
            {ICON_FA_WAND_MAGIC_SPARKLES, "ai"},
        };
        const float btn_width = 30;
        int btn_sz = buttons.size();
        float sp = ImGui::GetStyle().ItemSpacing.x;
        int n = (int)std::max(1.f, ImGui::GetContentRegionAvail().x / (btn_width + sp));
        bool selected;
        int column = min(n, btn_sz);
        for (size_t i = 0; i < btn_sz; i++)
        {
            ImGui::PushFont(mdicon);
            ImGui::PushID(i);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0.5f, 0.9f });
            ImGui::BeginDisabled( i == 6 && ollama_result.valid());
            selected = (i == 7) && !disable_continuously_generate;
            if (ImGui::Selectable(buttons.at(i).first.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(btn_width, btn_width)))
            {
                //button process begin
                if ( i == 0 ){ sm.copy_selected(); }
                else if ( i == 1 ){ sm.paste_selected(); }
                else if ( i == 2 ){ sm.delete_selected(); }
                else if ( i == 3 ){sm.lock_focused_node();}
                else if( i == 4 ){sm.unlock_focused_node();}
                else if( i == 5 ){
                    if( selected_node_count > 0){
                        string filepath = choose_path();
                        if( filepath.size() > 0 ){
                            texture_t texture = load_texture_from_file(filepath);
                            if( texture.first != 0 && ImLength(texture.second) > 0 ){
                                GE.execute_command(make_unique<BNImage>(G, texture));
                                should_update_graph_attributes = true;
                            }
                        }
                    }
                }
                else if( i == 6 ){//generate once
                    if( focused_node() != nullptr && util::is_valid_url(ollama_url)  && !ollama_model.empty() ) {
                        ollama_result = async(std::launch::async, [&](){
                            ollama_return_t ret;
                            GE.execute_command(make_unique<CallOllamaCommand>(G, ollama_url, ollama_model, [&](vector<string>& ns, vector<string>& es, node root){
                                ret.root = root; ret.nodes = ns; ret.edges = es;
                            }));
                            return ret;
                        });
                    }
                }
                else if( i == 7 ){//start continuously generate
                    if( disable_continuously_generate && G.leaf() != nullptr && util::is_valid_url(ollama_url)  && !ollama_model.empty() ) {
                        ollama_result = async(std::launch::async, [&](){
                            ollama_return_t ret;
                            GE.execute_command(make_unique<CallOllamaCommand>(G, ollama_url, ollama_model, [&](vector<string>& ns, vector<string>& es, node root){
                                ret.root = root; ret.nodes = ns; ret.edges = es;
                            }, G.leaf()));
                            return ret;
                        });
                    }
                    disable_continuously_generate = !disable_continuously_generate;
                }
                //button process end.
            }
            ImGui::EndDisabled();
            ImGui::PushFont(ImGui::GetDefaultFont());
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) )
                ImGui::SetTooltip("%s", buttons.at(i).second.c_str());
            ImGui::PopFont();
            if( i%column < column-1 && i != btn_sz-1 ) ImGui::SameLine();
            ImGui::PopStyleVar();
            ImGui::PopID();
            ImGui::PopFont();

            if (i == 6 && !ollama_result.valid() && ImGui::BeginPopupContextItem())
            {
                static std::vector<std::string> items = {};
                if( server_list.find(ollama_url) != server_list.end() ) {
                    if(server_list[ollama_url]->ready()) { items = server_list[ollama_url]->data(); } else { items.clear(); }
                }
                static int i = 0;
                std::string models="";
                for(auto s: items){ models.append(s+"\0"s);}
                if(ImGui::Combo("##ollama models", &i, models.c_str())){ ollama_model = items[i]; }
                ImGui::SameLine();
                if( ImGui::Button("fetch models") ){
                    if( util::is_valid_url(ollama_url) ){
                        server_list[ollama_url] = std::make_shared<OllamaServer>(ollama_url);
                        ollama_model.clear();
                    }
                }
                if(ImGui::InputTextWithHint("ollama server", "http://localhost:11434", &ollama_url, ImGuiInputTextFlags_CallbackHistory, [](ImGuiInputTextCallbackData* data)->int{
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
                    {
                        std::vector<std::string> history;
                        for(auto j: server_list) history.push_back(j.first);
                        int max = history.size()-1;
                        static int i = 0;
                        if (data->EventKey == ImGuiKey_UpArrow)
                        {
                            i = std::clamp(i+1, 0, max);
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, history[i].c_str());
                            data->SelectAll();
                        }
                        else if (data->EventKey == ImGuiKey_DownArrow)
                        {
                            i = std::clamp(i-1, 0, max);
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, history[i].c_str());
                            data->SelectAll();
                        }
                    }
                    return 0;
                })) {  }
                ImGui::EndPopup();
            }
        }

        selected_node_count = sm.count_selected_node();
        selected_edge_count = sm.count_selected_edge();
        selected_cluster_count = sm.count_selected_cluster();
        selected_count = selected_node_count + selected_edge_count + selected_cluster_count;
        only_one_object_selected = selected_count == 1;
        different_type_selected = selected_node_count && selected_edge_count || selected_node_count && selected_cluster_count || selected_edge_count && selected_cluster_count;
        if(selected_count && selected_count == selected_node_count){//only nodes selected
            //intialize color style with selected node's color style
            node n = focused_node();
            assert(n != nullptr);
            ImColor background_col = ImColor(n->m_bg_col);
            ImColor border_col = ImColor(n->m_bd_col);
            ImColor text_col = ImColor(n->m_txt_col);
            
            // update selected node's color style
            ImGui::PushFont(ImGui::GetDefaultFont());
            ImGui::SameLine();
            bool node_bg_col_changed = ImGui::ColorEdit4("##node_background_col", (float*)&background_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Background color");
            ImGui::SameLine();
            bool node_txt_col_changed = ImGui::ColorEdit4("##node_text_col", (float*)&text_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Text color");
            ImGui::SameLine();
            bool node_bd_col_changed = ImGui::ColorEdit4("##node_border_col", (float*)&border_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Border color");
            ImGui::PopFont();
            if(node_bg_col_changed || node_txt_col_changed || node_bd_col_changed){
                for(auto& n:G.nodes()){
                    if(n.second->m_focused){
                        n.second->m_bg_col = node_bg_col_changed?ImU32(background_col):n.second->m_bg_col;
                        n.second->m_bd_col = node_bd_col_changed?ImU32(border_col):n.second->m_bd_col;
                        n.second->m_txt_col = node_txt_col_changed?ImU32(text_col):n.second->m_txt_col;
                    }
                }
            }
        }else if(selected_count && selected_count == selected_edge_count){//only edges selected
            edge e = focused_edge();
            assert(e != nullptr);
            ImColor edge_col(e->m_edge_col);
            ImColor edge_text_col(e->m_txt_col);
            ImGui::PushFont(ImGui::GetDefaultFont());
            ImGui::SameLine();
            bool edge_col_changed = ImGui::ColorEdit4("##edge_col", (float*)&edge_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Edge color");
            ImGui::SameLine();
            bool edge_text_col_changed = ImGui::ColorEdit4("##edge_text_col", (float*)&edge_text_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Text color");
            ImGui::PopFont();
            if( edge_col_changed || edge_text_col_changed ){
                for(edge_pair e:G.edges()){
                    if(e.second->m_focused){
                        e.second->m_edge_col = edge_col_changed?ImU32(edge_col):e.second->m_edge_col;
                        e.second->m_txt_col = edge_text_col_changed?ImU32(edge_text_col):e.second->m_txt_col;
                    }
                }
            }
        }else if(selected_count && selected_count == selected_cluster_count){//only clusters selected
            cluster c = focused_cluster();
            assert(c != nullptr);
            ImColor cluster_bg_col(c->m_bg_col);
            ImColor cluster_txt_col(c->m_txt_col);
            ImColor cluster_border_col(c->m_bd_col);
            ImGui::PushFont(ImGui::GetDefaultFont());
            ImGui::SameLine();
            bool cluster_bg_col_changed = ImGui::ColorEdit4("##cluster_bg_col", (float*)&cluster_bg_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Background color");
            ImGui::SameLine();
            bool cluster_txt_col_changed = ImGui::ColorEdit4("##cluster_txt_col", (float*)&cluster_txt_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Text color");
            ImGui::SameLine();
            bool cluster_border_col_changed = ImGui::ColorEdit4("##cluster_border_col", (float*)&cluster_border_col, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) ) ImGui::SetTooltip("Border color");
            ImGui::PopFont();
            if( cluster_bg_col_changed || cluster_txt_col_changed || cluster_border_col_changed ){
                for(auto& c:G.clusters()){
                    if(c.second == G.root_cluster()) continue;
                    if(c.second->m_focused){
                        c.second->m_bg_col = cluster_bg_col_changed?ImU32(cluster_bg_col):c.second->m_bg_col;
                        c.second->m_txt_col = cluster_txt_col_changed?ImU32(cluster_txt_col):c.second->m_txt_col;
                        c.second->m_bd_col = cluster_border_col_changed?ImU32(cluster_border_col):c.second->m_bd_col;
                    }
                }
            }
        }

        // ImGui::EndChild();
        ImGui::PopFont();
    }

    if ( ollama_result.valid() && cola_inprogress == 0){
        if ( std::future_status::ready == ollama_result.wait_for(std::chrono::milliseconds(1)) ){
            ollama_return_t result = ollama_result.get();
            if( result.root != nullptr && result.nodes.size() > 0 && result.edges.size() > 0){
                for (size_t i = 0; i < result.nodes.size(); i++)
                {
                    node child = nullptr;
                    ImVec2 cp = util::random_point(result.root->m_rect.GetCenter(), 4 * ImLength(result.root->m_rect.GetSize()));
                    auto cmd1 = make_unique<CreateNodeCommand>(G, cp ,[&](node n){
                        ImGui::PushFont(node_font);
                        string label = result.nodes[i];
                        auto tsz = ImGui::CalcTextSize(label.c_str(), NULL, false, 0.0f);
                        ImGui::PopFont();
                        n->m_label = label;
                        n->m_rect = ImRect(cp-0.5f*tsz, cp+0.5f*tsz);
                        n->m_rect.Expand(node_text_margins);
                        n->m_bd_col = node_border_col;
                        n->m_bg_col = node_fill_col;
                        n->m_txt_col = node_text_col;
                        n->m_image = 0;
                        n->m_image_rect.Max = n->m_image_rect.Min = {};
                        child = n;
                    });
                    GE.execute_command(std::move(cmd1));
                    auto cmd2 = make_unique<CreateEdgeCommand>(G, result.root, child, [&](edge e){
                        e->m_label = result.edges[i];
                        e->m_arrow = arrow_e::normal;
                        e->m_ideal_length = 350.0f;
                        e->m_edge_col = edge_col;
                        e->m_txt_col = node_text_col;
                        e->m_label_position = 0.6f;
                    });
                    GE.execute_command(std::move(cmd2));
                }
                // layout_once = true;
                // route_once = true;
                if( !disable_continuously_generate && G.leaf() != nullptr && util::is_valid_url(ollama_url)  && !ollama_model.empty() ) {
                    ollama_result = async(std::launch::async, [&](){
                        ollama_return_t ret;
                        GE.execute_command(make_unique<CallOllamaCommand>(G, ollama_url, ollama_model, [&](vector<string>& ns, vector<string>& es, node root){
                            ret.root = root; ret.nodes = ns; ret.edges = es;
                        }, G.leaf()));
                        return ret;
                    });
                }
            } else {

            }
        }
        else{
            ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(0.0f, 0.0f), "Ollama..");
        }
    }

    if( selected_count == 0 || different_type_selected )//nothing selected or multiple types selected
    {
        ImGui::MarkdownConfig mdConfig{ NULL, NULL, NULL, ICON_FA_LINK, { { H1, true }, { H2, true }, { H3, false } }, NULL }; 
        const string help = u8R"(Tips:
___
  * Double-click blank aera to create a node.
  * Click on blank area to deselect.
  * Click on node to select node.
  * Click on edge to select edge.
  * Click on group to select group.
  * Hold CTRL + click to select continuously.
  * Node(s) selected + press Shift + click on node to create edge(s).
  * Node(s) selected + press Alt to create group.
  * Drag a selection box on the canvas to select all nodes within the box.
  * Drag on node to move node.
  * Drag on group to move group.
  * Double click object to edit it's label.
  * CTRL + A to select all.
  * CTRL + C to copy.
  * CTRL + V to paste.
  * The Copy command copies only the selected nodes and the edges between them, if any.
  * CTRL + Z to undo.
  * CTRL + Y to redo.
  * Use locking if you do not want the auto-layout algorithm to change the position of a node or group.
  * You can remove bending points on an edge by right-clicking the item in the edge table then click "reset" in the context menu.
)";

        ImGui::BeginChild("help", ImVec2(-FLT_MIN, -1));
        ImGui::Markdown(help.c_str(), help.length(), mdConfig);
        ImGui::EndChild();
        
    }
    else if( only_one_object_selected && selected_node_count == 1 )//only 1 node selected
    {
        node n = focused_node();
        assert(n != nullptr);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_AutoSelectAll;
        ImGui::PushFont(H3);
        if( active_node_label_editor)ImGui::ActivateItemByID(ImGui::GetID("##node_label_input"));
        node_label_changed = ImGui::InputTextMultiline("##node_label_input", &n->m_label, ImVec2(-FLT_MIN, -1), flags);
        want_editing = ImGui::IsItemFocused();
        if(ImGui::IsItemActivated()){
            ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetID("##node_label_input"));
            if(state) state->SelectAll();
        }
        ImGui::PopFont();
    }
    else if( only_one_object_selected && selected_edge_count == 1 )//only 1 edge selected
    {
        edge e = focused_edge();
        assert(e != nullptr);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_AlwaysOverwrite;
        ImGui::PushFont(H3);
        if( active_edge_label_editor)ImGui::ActivateItemByID(ImGui::GetID("##edge_label_input"));
        edge_label_changed = ImGui::InputTextMultiline("##edge_label_input", &e->m_label, ImVec2(-FLT_MIN, -1), flags);
        want_editing = ImGui::IsItemFocused();
        if(ImGui::IsItemActivated()){
            ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetID("##edge_label_input"));
            if(state) state->SelectAll();
        }
        ImGui::PopFont();
    }
    else if( only_one_object_selected && selected_cluster_count == 1 )//only 1 cluster selected
    {
        cluster c = focused_cluster();
        assert(c != nullptr);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_AlwaysOverwrite;
        ImGui::PushFont(H3);
        if( active_group_label_editor)ImGui::ActivateItemByID(ImGui::GetID("##cluster_label_input"));
        cluster_label_changed = ImGui::InputTextMultiline("##cluster_label_input", &c->m_label, ImVec2(-FLT_MIN, -1), flags);
        want_editing = ImGui::IsItemFocused();
        if(ImGui::IsItemActivated()){
            ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetID("##cluster_label_input"));
            if(state) state->SelectAll();
        }
        ImGui::PopFont();
    }

    ImGui::End(); 
    
    //==================================================Edit Properties End==================================================

    //==================================================Layout Config Begin=============================================
    static std::future<void> layout_result = {};
    static std::future<void> route_result = {};
    static LayoutProgress layout_process;
    static cola::CompoundConstraints ccs;
    static float layout_progress = 0.0f;
    static float route_progress = 0.0f;
    static RouteProcess *routeprocess = nullptr;
    static vector<Avoid::ConnRef *> connRefs;
    static int routing_type = 0;
    static vector<pair<string, pair<Avoid::RoutingParameter, pair<string, float>>>> routing_parameters = {
   {"Segment Penalty",{ Avoid::segmentPenalty, {R"(@brief
   This penalty is applied for each segment in the connector 
   path beyond the first.  This should always normally be set
   when doing orthogonal routing to prevent step-like connector
   paths.
@note
   This penalty must be set (i.e., be greater than zero) in 
   order for orthogonal connector nudging to be performed, since
   this requires reasonable initial routes. )", 50}}},
   {"Angle Penalty",{Avoid::anglePenalty, {R"(@brief
   This penalty is applied in its full amount to tight acute 
   bends in the connector path.  A smaller portion of the penalty
   is applied for slight bends, i.e., where the bend is close to
   180 degrees.  This is useful for polyline routing where there
   is some evidence that tighter corners are worse for 
   readability, but that slight bends might not be so bad, 
   especially when smoothed by curves.)", 50}}},
   {"Shape Buffer Distance",{Avoid::shapeBufferDistance, {R"(@brief
  This parameter defines the spacing distance that will be added
  to the sides of each shape when determining obstacle sizes for
  routing.  This controls how closely connectors pass shapes, and
  can be used to prevent connectors overlapping with shape 
  boundaries. By default, this distance is set to a value of 0.)", 5}}},
   {"Ideal Nudging Distance",{Avoid::idealNudgingDistance, {R"(@brief
  This parameter defines the spacing distance that will be used
  for nudging apart overlapping corners and line segments of 
  connectors.  By default, this distance is set to a value of 4.)", 4.0}}},

    };
    bool layout_setting_changed = false;
    bool layout_inprogress = layout_result.valid();
    bool route_inprogress = route_result.valid();
    const float maxiterations = layout_process.maxiterations;
    layout_progress = layout_process.iterations/maxiterations;
    
    auto add_cola_alignment = [&](vpsc::Dim dim_a, vpsc::Dim dim_s, cola_alignment alignment)->bool{
        int focused_count = sm.count_selected_node();
        if( focused_count <2 ) return false;
        double offset = 0;
        cola::AlignmentConstraint *ac = new cola::AlignmentConstraint(dim_a);//align x-axis
        ccs.push_back(ac);
        node last_node = nullptr;
        for(auto i:G.nodes()){
            if( i.second->m_focused){
                ImRect rect = i.second->m_rect;
                switch ( alignment )
                {
                case cola_alignment::align_center:
                    offset = 0;
                    break;
                case cola_alignment::align_left:
                    offset = 0.5f * ((dim_a == vpsc::Dim::XDIM)?rect.GetWidth():rect.GetHeight());
                    break;
                case cola_alignment::align_right:
                    offset = -0.5f * ((dim_a == vpsc::Dim::XDIM)?rect.GetWidth():rect.GetHeight());
                    break;
                default:
                    break;
                }
                ac->addShape(i.second->m_seq, offset);
                if(last_node != nullptr)
                {
                    ImRect last_rect = last_node->m_rect;
                    float space = (dim_s == vpsc::Dim::XDIM?((last_rect.GetWidth() + rect.GetWidth()) * (0.5f + alignment_space)):((last_rect.GetHeight() + rect.GetHeight()) * (0.5f + alignment_space)));
                    ccs.push_back(new cola::SeparationConstraint(dim_s, last_node->m_seq, i.second->m_seq, space, true));
                }
                last_node = i.second;
            };
        }
        return true;
    };
    auto add_tree_constraint = [&]() -> bool {
        if( !G.empty() ){
            //find node(s) with zero in-degree and non-zero out-degree as root(s)
            node root = G.choose_node([&](node n){ return n->m_indeg == 0 && n->m_outdeg > 0; });
            if( root == nullptr ) return false;
            auto bs = G.descendants(root);
            for(int i = 1; i< bs.size(); i++) {
                auto& last_br = bs[i-1];
                auto& this_br = bs[i];
                if( last_br.back() == this_br.front() ){
                    last_br.insert(last_br.end(), make_move_iterator(++this_br.begin()), make_move_iterator(this_br.end()));
                }
            }
            vector<pair<cola::AlignmentConstraint*, float>> ac_refs;
            for( auto b:bs )
            {
                cola::AlignmentConstraint *ac = new cola::AlignmentConstraint(vpsc::Dim::YDIM);
                ccs.push_back(ac);
                float max_height = 0.0f;
                for (size_t i = 1; i < b.size(); i++){
                    ac->addShape(b[i].second,0.0);
                    node n0 = G.choose_node([&](node n){ return n->m_seq == b[i-1].second; });assert(n0 != nullptr);
                    node n1 = G.choose_node([&](node n){ return n->m_seq == b[i].second; });assert(n1 != nullptr);
                    float space = (n0->m_rect.GetWidth() + n1->m_rect.GetWidth()) * 0.5f + 70;
                    ccs.push_back(new cola::SeparationConstraint(vpsc::Dim::XDIM, b[i-1].second, b[i].second, space, true));
                    if(i == 1)n1->m_locked = true;
                    max_height = max(max_height, max(n0->m_rect.GetHeight(), n1->m_rect.GetHeight()));
                }
                ac_refs.push_back(make_pair(ac, max_height));
            }
            cola::AlignmentConstraint *ac = new cola::AlignmentConstraint(vpsc::Dim::XDIM);
            for (size_t i = 1; i < bs.size(); i++){
                if(bs[i].size() <= 1 || bs[i-1].size() <= 1)continue;
                auto last_br = bs[i-1];
                auto this_br = bs[i];
                for(auto j: last_br){
                    if( j.first == this_br.front().first ){
                        ac = new cola::AlignmentConstraint(vpsc::Dim::XDIM);
                        ac->addShape(j.second, 0.0);
                        ac->addShape(this_br.front().second, 0.0);
                    }
                }
            }
            for (size_t i = 1; i < ac_refs.size(); i++)
            {
                cola::DistributionConstraint *dc = new cola::DistributionConstraint(vpsc::Dim::YDIM);
                dc->addAlignmentPair(ac_refs[i-1].first, ac_refs[i].first);
                dc->setSeparation((ac_refs[i-1].second + ac_refs[i].second)*0.5f + 40);
                ccs.push_back(dc);
            }
            return true;
        }
        return false;
    };
    
    ImGui::Begin("Layout");
    ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
    if (ImGui::CollapsingHeader("General"))
    {
        if(layout_result.valid()){ImGui::SameLine();ImGui::ProgressBar(layout_progress);}
        if(route_result.valid()){ImGui::SameLine();ImGui::ProgressBar(route_progress);}
        vector<pair<string, string>> buttons = {
            {ICON_FA_FILE_EXPORT, "export."},
            {ICON_FA_FILE_IMPORT, "import."},
            {ICON_FA_ROUTE, "compute route once."},
            {ICON_FA_VECTOR_SQUARE, "compute layout once."},
            {ICON_FA_DICE, "generate random graph"},
            {ICON_MD_ALIGN_HORIZONTAL_LEFT, "align-horizontal-left."},
            {ICON_MD_ALIGN_HORIZONTAL_CENTER, "align-horizontal-center."},
            {ICON_MD_ALIGN_HORIZONTAL_RIGHT, "align-horizontal-right."},
            {ICON_MD_ALIGN_VERTICAL_TOP, "align-vertical-top."},
            {ICON_MD_ALIGN_VERTICAL_CENTER, "align-vertical-center."},
            {ICON_MD_ALIGN_VERTICAL_BOTTOM, "align-vertical-bottom."},
            {ICON_MD_ACCOUNT_TREE, "tree"},
            {ICON_FA_BUG, "toggle debug window."},
        };
        
        const float btn_width = 30;
        int btn_sz = buttons.size();
        float sp = ImGui::GetStyle().ItemSpacing.x;
        int n = (int)std::max(1.f, ImGui::GetContentRegionAvail().x / (btn_width + sp));
        bool selected;
        int column = min(n, btn_sz);
        for (size_t i = 0; i < btn_sz; i++)
        {
            bool should_disable = false;
            should_disable |= ( i == 2 )&&(route_inprogress || enable_continue_route);
            should_disable |= ( i >= 3 && i < 12)&&(layout_inprogress || enable_continue_layout);
            selected = (i == 12)?show_debug_window:false;
            ImGui::PushFont(icon);
            ImGui::PushID(i);
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0.5f, 0.9f });
            ImGui::BeginDisabled(should_disable);
            if (ImGui::Selectable(buttons.at(i).first.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(btn_width, btn_width)))
            {
                //button logic begin
                if ( i == 0 )//export file
                {
                    GE.execute_command(make_unique<ExportCommand>(G)); 
                }
                else if ( i == 1 )//import file
                {
                    GE.execute_command(make_unique<ImportCommand>(G));
                }
                else if ( i == 2 )//route
                {
                    route_once = true;
                }
                else if ( i == 3 )//layout
                {
                    layout_once = true;
                }
                else if( i == 4 )//random graph
                {
                    G.generate_random_graph(40, 100, 10);
                    update_graph_attributes(true);
                    layout_once = true;
                }
                else if( i == 5 )//align-horizontal-left
                {
                    layout_once = add_cola_alignment(vpsc::Dim::XDIM, vpsc::Dim::YDIM, cola_alignment::align_left);
                }
                else if( i == 6 )//align-horizontal-center
                {
                    layout_once = add_cola_alignment(vpsc::Dim::XDIM, vpsc::Dim::YDIM, cola_alignment::align_center);
                }
                else if( i == 7 )//align-horizontal-right
                {
                    layout_once = add_cola_alignment(vpsc::Dim::XDIM, vpsc::Dim::YDIM, cola_alignment::align_right);
                }
                else if( i == 8 )//align-vertical-top
                {
                    layout_once = add_cola_alignment(vpsc::Dim::YDIM, vpsc::Dim::XDIM, cola_alignment::align_left);
                }
                else if( i == 9 )//align-vertical-center
                {
                    layout_once = add_cola_alignment(vpsc::Dim::YDIM, vpsc::Dim::XDIM, cola_alignment::align_center);
                }
                else if( i == 10 )//align-vertical-bottom
                {
                    layout_once = add_cola_alignment(vpsc::Dim::YDIM, vpsc::Dim::XDIM, cola_alignment::align_right);
                }
                else if( i == 11 )
                {
                    layout_once = add_tree_constraint();
                }
                else if( i == 12 )
                {
                    show_debug_window = !show_debug_window;
                }
                //button logic end.
            }
            ImGui::EndDisabled();
            ImGui::PushFont(ImGui::GetDefaultFont());
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) )
                ImGui::SetTooltip("%s", buttons.at(i).second.c_str());
            ImGui::PopFont();
            if( i%column < column-1 && i != btn_sz-1 ) ImGui::SameLine();
            ImGui::PopStyleVar();
            ImGui::PopID();
            ImGui::PopFont();
            if( i == 2 && ImGui::BeginPopupContextItem() ) {
                if( ImGui::Button("reset") ) {
                    G.clear_all_bendpoint();
                }
                ImGui::EndPopup();
            }
        }


        // ImGui::TreePop();
    }

    // ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
    if (ImGui::CollapsingHeader("Layout settings"))
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
        if(ImGui::TreeNode("layouting")){
            ImGui::BeginDisabled(layout_inprogress);
            layout_setting_changed |= ImGui::Checkbox("layout continuously", &enable_continue_layout);
            ImGui::EndDisabled();
            ImGui::BeginDisabled(route_inprogress);
            layout_setting_changed |= ImGui::Checkbox("routing continuously", &enable_continue_route);
            ImGui::EndDisabled();
            layout_setting_changed |= ImGui::Checkbox("tree mode", &enable_tree_mode);
            if(enable_tree_mode){
                ccs.clear();
                layout_setting_changed |= add_tree_constraint();
            }
            layout_setting_changed |= ImGui::DragFloat("edge scale", &ideal_length, 0.01f, 0.0f, 0.0f);
            layout_setting_changed |= ImGui::DragFloat("cluster margin", &cluster_margin, 0.0f, 0.1f, 500.0f, "%.1f pixel");
            ImGui::DragFloat("guideline search threshold", &guideline_search_threshold, 1.0f, 0.0f, 0.0f);
            ImGui::DragFloat("guideline snap threshold", &guideline_snap_threshold, 1.0f, 0.0f, 0.0f);
            ImGui::TreePop();
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
        if(ImGui::TreeNode("routing"))
        {
            if (ImGui::Combo("Mode", &routing_type, "Polyline Routing\0Orthogonal Routing\0")){}
            for (size_t i = 0; i < routing_parameters.size(); i++)
            {
                ImGui::DragFloat(routing_parameters[i].first.c_str(), &routing_parameters[i].second.second.second, 1.0f, 1.0f, FLT_MAX);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) )
                    ImGui::SetTooltip("%s", routing_parameters.at(i).second.second.first.c_str());
            }
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Style"))
    {
        static int style_idx = 1;
        if (ImGui::Combo("Themes", &style_idx, "Dark\0Light\0Classic\0"))
        {
            switch (style_idx)
            {
            case 0: ImGui::StyleColorsDark(); break;
            case 1: ImGui::StyleColorsLight(); break;
            case 2: ImGui::StyleColorsClassic(); break;
            }
            node_text_col = ImGui::GetStyle().Colors[MindMap_Node_Text];
            node_border_col = ImGui::GetStyle().Colors[MindMap_Node_Border];
            node_fill_col = ImGui::GetStyle().Colors[MindMap_Node_Bg];
            edge_col = ImGui::GetStyle().Colors[MindMap_Edge];
            select_box_fill_col = ImGui::GetStyle().Colors[MindMap_Rect_Bg];
            focused_col = ImGui::GetStyle().Colors[MindMap_Focused];
            lock_icon_col = ImGui::GetStyle().Colors[MindMap_Lock_Icon];
            linking_col = ImGui::GetStyle().Colors[MindMap_Edge_Linking];
            cluster_background_col = ImGui::GetStyle().Colors[MindMap_Cluster_Bg];
            cluster_focused_col = ImGui::GetStyle().Colors[MindMap_Cluster_Focused];

        }
        should_update_graph_attributes |= ImGui::Combo("Contact point type", &contact_point_type, "Direct\0Closest\0");
        ImGui::Combo("flow direction", &flow_direction, "flow into focused node\0flow outof focused node\0");
        ImGui::Combo("auto active label editor", &auto_active_label_editor, "none\0on double click\0on creat\0both\0");
        ImGui::DragFloat2("node size", &node_sz.x);
        node_label_changed |= ImGui::DragFloat("node text margins", &node_text_margins, 1.0f, 1.0f, 200.0f);
        ImGui::ColorEdit4("node text", (float*)&node_text_col);
        ImGui::ColorEdit4("node border", (float*)&node_border_col);
        ImGui::ColorEdit4("node background", (float*)&node_fill_col);
        ImGui::ColorEdit4("edge line", (float*)&edge_col);
        ImGui::ColorEdit4("lock icon", (float*)&lock_icon_col);
        ImGui::ColorEdit4("focused node", (float*)&focused_col);
        ImGui::ColorEdit4("cluster focused", (float*)&cluster_focused_col);
        ImGui::ColorEdit4("cluster background", (float*)&cluster_background_col);
        ImGui::DragFloat("border thickess", &border_thickness, 0.1f, 0.1f, FLT_MAX);
        should_update_graph_attributes |= ImGui::DragFloat("arrow head size", &arrow_head_sz, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("arrow head thickness", &arrow_head_thickness, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("arrow line thickness", &arrow_line_thickness, 0.1f, 1.0f, 10.0f);
        should_update_graph_attributes |= ImGui::DragFloat("arrow angle", &arrow_angle, 0.1f, 5.0f, 80.0f);
        ImGui::DragFloat("edge proximity threshold", &proximity_threshold, 0.1f, 1.0f, 40.0f);
        ImGui::DragFloat("canvas panning speed", &canvas_panning_speed, 1.0f, 1.0f, 5000.0f,"%.1f pixel/sec");
        ImGui::DragFloat("dot size", &dot_sz, 0.1f, 1.0f, 10.0f);
        ImGui::DragFloat("dot ideal gap", &ideal_dot_gap, 1.0f, 1.0f, 500.0f);
        ImGui::DragFloat("dot flow speed", &flow_speed, 0.0f, 0.1f, 500.0f, "%.1f pixel/sec");
        should_update_graph_attributes |= ImGui::DragFloat("cluster padding", &cluster_padding, 0.0f, 0.1f, 500.0f, "%.1f pixel");
    }
    auto pre_layout = [](vector<vpsc::Rectangle*>& nodes, vector<cola::Edge>& edges, cola::Locks& locks, cola::EdgeLengths &eLengths, cola::RootCluster*& root){
        nodes.clear();
        edges.clear();
        locks.clear();
        eLengths.clear();
        for(auto n:G.nodes()){
            ImRect &r = n.second->m_rect;
            nodes.push_back(new vpsc::Rectangle(r.Min.x, r.Max.x, r.Min.y, r.Max.y));
        }

        for(auto e= G.edges().begin(); e != G.edges().end();++e)
        {
            cola::Edge cola_e(e->second->m_source->m_seq, e->second->m_target->m_seq);
            edges.push_back(cola_e);
        }
        if(G.clusters().size() > 1){
            root = new cola::RootCluster();
            root->setAllowsMultipleParents(true);//allow cluster overlap
            for( auto n: G.root_cluster()->m_child_nodes ){
                root->addChildNode(n->m_seq);
            }
            for( auto c: G.clusters() ){
                if( c.second == G.root_cluster() ) continue;
                cola::RectangularCluster *rc = new cola::RectangularCluster();
                rc->setMargin(c.second->m_margin);
                rc->setPadding(c.second->m_padding);
                for( auto n: c.second->m_child_nodes ){
                    rc->addChildNode(n->m_seq);
                }
                root->addChildCluster(rc);
            }
        }
        for(auto i:G.nodes()){ if ( i.second->m_locked ){locks.push_back(cola::Lock(i.second->m_seq, i.second->m_rect.GetCenter().x, i.second->m_rect.GetCenter().y));}}
        for(auto i:G.edges()){ eLengths.push_back(i.second->m_ideal_length); }
        cola_inprogress++;
    };
    auto post_layout = [](LayoutProgress& progress){
        if(!enable_continue_layout) GE.execute_command(make_unique<AutoLayoutCommand>(G, progress.centers));
        int i = 0;
        for(auto& n: G.nodes())
        {
            n.second->m_rect.Translate(progress.centers[i] - n.second->m_rect.GetCenter());
            i++;
        }
        should_update_graph_attributes  = true;
        cola_inprogress--;
    };
    auto pre_route = [](Avoid::Router *router, vector<Avoid::ConnRef *> &connrefs){
        for(auto n: G.nodes())
        {
            ImRect r = n.second->m_rect;
            Avoid::Rectangle rectangle(Avoid::Point(r.GetCenter().x, r.GetCenter().y), r.GetWidth(), r.GetHeight());
            Avoid::ShapeRef *shapeRef = new Avoid::ShapeRef(router, rectangle); 
        }
        for(auto c:G.clusters())
        {
            if(c.second != G.root_cluster())
            {
                Avoid::Rectangle rectangle(Avoid::Point(c.second->m_rect.GetCenter().x, c.second->m_rect.GetCenter().y), c.second->m_rect.GetWidth(), c.second->m_rect.GetHeight());
                Avoid::ShapeRef *shapeRef = new Avoid::ShapeRef(router, rectangle); 
            }
        }
        
        for(auto e:G.edges())
        {
            Avoid::Point newSrcPt(e.second->m_source->m_rect.GetCenter().x, e.second->m_source->m_rect.GetCenter().y);
            Avoid::Point newDstPt(e.second->m_target->m_rect.GetCenter().x, e.second->m_target->m_rect.GetCenter().y);
            Avoid::ConnRef *connref = new Avoid::ConnRef(router, newSrcPt, newDstPt);
            connrefs.push_back(connref);
        }
        cola_inprogress++;
    };
    auto post_route = [](vector<Avoid::ConnRef *> &connrefs){
        int i = 0;
        for(auto& e: G.edges())
        {
            e.second->m_bends.clear();
            for (size_t j = 1; j < connrefs[i]->displayRoute().size()-1; j++)
            {
                Avoid::Point point = connrefs[i]->displayRoute().at(j);
                e.second->m_bends.push_back(ImVec2(point.x, point.y));
            }
            i++;
        }
        should_update_graph_attributes  = true;
        cola_inprogress--;
    };
    //check if need layout
    static int layout_check = 0;
    bool graph_attributes_changed = (G.changed(layout_check) || lengths_changed || label_positions_changed || node_label_changed || edge_label_changed || cluster_label_changed || is_moving);
    should_update_graph_attributes |= graph_attributes_changed;
    bool should_layout =  enable_continue_layout && (layout_setting_changed || graph_attributes_changed);
    if ( should_layout )
    {
        vector<cola::Edge> cola_edges;
        vector<vpsc::Rectangle*> cola_nodes;
        cola::EdgeLengths eLengths;
        cola::Locks locks;
        SetDesiredPos desired(locks);
        LayoutProgress progress;
        cola::RootCluster *root_cluster = nullptr;
        pre_layout(cola_nodes, cola_edges, locks, eLengths, root_cluster);
        cola::ConstrainedFDLayout layout(cola_nodes, cola_edges, ideal_length, eLengths, &progress, &desired);
        if(root_cluster)layout.setClusterHierarchy(root_cluster);
        layout.setAvoidNodeOverlaps(true);
        layout.setConstraints(ccs);
        layout.run();
        post_layout(progress);
        for_each(ccs.begin(), ccs.end(), cola::delete_object());
        ccs.clear();
        if(root_cluster)delete root_cluster;
    }

    if ( layout_once )
    {
        layout_once = false;
        layout_result = async(std::launch::async, [&](){
            layout_process.clear();
            vector<cola::Edge> cola_edges;
            vector<vpsc::Rectangle*> cola_nodes;
            cola::EdgeLengths eLengths;
            cola::Locks locks;
            SetDesiredPos desired(locks);
            cola::RootCluster *root_cluster = nullptr;
            pre_layout(cola_nodes, cola_edges, locks, eLengths, root_cluster);
            cola::ConstrainedFDLayout layout(cola_nodes, cola_edges, ideal_length, eLengths, &layout_process, &desired);
            if(root_cluster)layout.setClusterHierarchy(root_cluster);
            layout.setAvoidNodeOverlaps(true);
            layout.setConstraints(ccs);
            layout.run();
            if(root_cluster) delete root_cluster;
        });
    }
    if ( layout_result.valid() )
    {
        if ( std::future_status::ready == layout_result.wait_for(std::chrono::milliseconds(1)) )
        {
            layout_result.get();
            post_layout(layout_process);
            layout_process.iterations = layout_process.maxiterations;
            for_each(ccs.begin(), ccs.end(), cola::delete_object());
            ccs.clear();
        }
    }

    if( enable_continue_route )
    {
        Avoid::Router *router = new Avoid::Router(static_cast<Avoid::RouterFlag>(routing_type+1));
        for(auto i: routing_parameters){ router->setRoutingParameter(i.second.first, i.second.second.second); }
        vector<Avoid::ConnRef *> connrefs;
        pre_route(router, connrefs);
        router->processTransaction();
        post_route(connrefs);
        delete router;
    }
    
    if( route_once )
    {
        route_once = false;
        route_progress = 0.0f;
        routeprocess = new RouteProcess(static_cast<Avoid::RouterFlag>(routing_type+1));
        for(auto i: routing_parameters){ routeprocess->setRoutingParameter(i.second.first, i.second.second.second); }
        pre_route(routeprocess, connRefs);
        route_result = async(std::launch::async, [&](){
            routeprocess->processTransaction();
        });
    }
    if ( route_result.valid() )
    {
        if(routeprocess) route_progress = routeprocess->progress;
        if ( std::future_status::ready == route_result.wait_for(std::chrono::milliseconds(1)) )
        {
            route_result.get();
            post_route(connRefs);
            connRefs.clear();
            delete routeprocess; 
            routeprocess = nullptr; 
        }
    }
    
    ImGui::End();
    //==================================================Layout Config End=============================================
}
