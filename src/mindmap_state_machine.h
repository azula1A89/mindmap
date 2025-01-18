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

#include <functional>

namespace state_machine{
using namespace std;
using namespace mindmap;

typedef struct state_machine_s
{
    //input
    node node_hovering = nullptr;
    edge edge_hovering = nullptr;
    cluster cluster_hovering = nullptr;
    bool left_down = false;
    bool left_released = true;
    bool left_clicked = false;
    bool left_double_clicked = false;
    bool left_dragging = false;
    bool ctrl_down = false;
    bool shift_down = false;
    bool alt_down = false;
    bool tab_down = false;
    bool enter_down = false;
    bool is_window_hovered = false;
    bool is_window_focused = false;
    bool want_select_all = false;
    bool want_delete = false;
    bool want_copy = false;
    bool want_paste = false;
    bool want_cut = false;
    bool want_undo = false;
    bool want_redo = false;
    ImVec2 mouse_pos{0,0};
    bool table_selected = false;

    //output & actions
    function<node(void)> create_node = nullptr;
    function<node(node p)> create_child = nullptr;
    function<node(node b)> create_sibling = nullptr;
    function<vector<edge>(vector<node> src, node dst)> create_edge = nullptr;
    function<cluster(vector<node>)> create_cluster = nullptr;
    function<void()> select_all_node = nullptr;
    function<void()> select_all_edge = nullptr;
    function<void()> select_all_cluster = nullptr;
    function<void(node)> toggle_select_node = nullptr;
    function<void(edge)> toggle_select_edge = nullptr;
    function<void(cluster)> toggle_select_cluster = nullptr;
    function<void(node)> single_select_node = nullptr;
    function<void(edge)> single_select_edge = nullptr;
    function<void(cluster)> single_select_cluster = nullptr;
    function<void(ImVec2, ImVec2)> aera_select_object = nullptr;
    function<void(void)> unselect_all_node = nullptr;
    function<void(void)> unselect_all_edge = nullptr;
    function<void(void)> unselect_all_cluster = nullptr;
    function<void()> move_node_begin = nullptr;
    function<void(ImVec2, ImVec2)> move_node_end = nullptr;
    function<void()> search_guidline = nullptr;
    function<void()> apply_guidline = nullptr;
    function<void()> move_cluster_begin = nullptr;
    function<void(ImVec2, ImVec2)> move_cluster_end = nullptr;
    function<void(node)> node_properties_popup = nullptr;
    function<void(edge)> edge_properties_popup = nullptr;
    function<void(cluster)> cluster_properties_popup = nullptr;
    function<node(void)> get_selected_node = nullptr;
    function<vector<node>(void)> get_selected_nodes = nullptr;
    function<void(void)> lock_focused_node = nullptr;
    function<void(void)> unlock_focused_node = nullptr;
    function<void(void)> draw_links = nullptr;
    function<int(void)> count_selected_node = nullptr;
    function<int(void)> count_selected_edge = nullptr;
    function<int(void)> count_selected_cluster = nullptr;
    function<bool(node)> is_node_focused = nullptr;
    function<bool(edge)> is_edge_focused = nullptr;
    function<bool(cluster)> is_cluster_focused = nullptr;
    function<void()> save_graph = nullptr;
    function<void()> delete_selected = nullptr;
    function<void()> copy_selected = nullptr;
    function<void()> paste_selected = nullptr;
    function<void()> cut_selected = nullptr;
    function<void()> undo_command = nullptr;
    function<void()> redo_command = nullptr;
    function<unsigned int(node)> is_hovering_pin = nullptr;
    function<void(unsigned int)> resize_image_begin = nullptr;
    function<void(ImVec2, ImVec2, unsigned int)> resize_image_end = nullptr;
    function<bool(void)> is_clipbord_data_valid = nullptr;
    const static int CREATE = 1<<0;
    const static int SELECT = 1<<1;
    const static int ADJUST = 1<<2;
    const static int ct_node = 1<<3;
    const static int ct_edge = 1<<4;
    const static int ct_cluster = 1<<5;
    const static int st_single = 1<<6;
    const static int st_multiple = 1<<7;
    const static int st_area = 1<<8;
    const static int at_moving_node = 1<<9;
    const static int at_moving_cluster = 1<<10;
    const static int ct_sibling = 1<<11;
    const static int ct_child = 1<<12;
    const static int at_resize_node = 1<<13;
    string debug_msg;

    void loop(){
        int selected_node_count = count_selected_node();
        int selected_edge_count = count_selected_edge();
        int selected_cluster_count = count_selected_cluster();
        int selected_count = selected_node_count + selected_edge_count + selected_cluster_count;
        bool different_type_selected = selected_node_count && selected_edge_count || selected_node_count && selected_cluster_count || selected_edge_count && selected_cluster_count;
        unsigned int pin_hovering = is_hovering_pin(get_selected_node());
        bool hovering_node = ( node_hovering != nullptr );
        bool hovering_edge = ( edge_hovering != nullptr );
        bool hovering_cluster = ( cluster_hovering != nullptr );
        bool hovering_node_pin = pin_hovering != 0;
        bool hovering_node_valid = is_window_hovered && hovering_node;
        bool hovering_edge_valid = is_window_hovered && hovering_edge;
        bool hovering_cluster_valid = is_window_hovered && hovering_cluster;
        bool hovering_node_pin_valid = is_window_hovered && hovering_node_pin;
        bool hovering = (hovering_edge || hovering_node || hovering_cluster || hovering_node_pin);
        bool released_node =  hovering_node_valid && left_released;
        bool released_edge = hovering_edge_valid && left_released;
        bool released_cluster = hovering_cluster_valid && left_released;
        bool click_somthing = is_window_hovered &&  hovering  && left_down;
        bool click_nothing = is_window_hovered && (!hovering) && left_clicked;
        bool double_clicked_nothing = is_window_hovered && (!hovering) && left_double_clicked;
        bool double_clicked_node = hovering_node_valid && left_double_clicked;
        bool double_clicked_edge = hovering_edge_valid && left_double_clicked;
        bool double_clicked_cluster = hovering_cluster_valid && left_double_clicked;
        bool dragging_node = (hovering_node_valid) && left_dragging;
        bool dragging_cluster = (hovering_cluster_valid) && left_dragging;
        bool dragging_nothing = is_window_hovered &&(!hovering) && left_dragging;
        bool dragging_node_pin = hovering_node_pin_valid && left_dragging;
        bool dragging_valid = is_window_focused && left_dragging;
        bool ctrl_down_valid = is_window_hovered && ctrl_down;
        bool ctrl_up_valid = is_window_hovered && !ctrl_down;
        bool shift_down_valid = is_window_hovered && shift_down;
        bool alt_down_valid = is_window_hovered && alt_down;
        bool alt_up_valid = is_window_hovered && !alt_down;
        bool want_delete_valid = selected_count && want_delete;
        bool want_redo_valid =  want_redo;
        bool want_undo_valid =  want_undo;
        bool want_copy_valid =  selected_count && want_copy;
        bool want_paste_valid =  want_paste && is_clipbord_data_valid();
        bool want_cut_valid =  selected_count && want_cut;

        static int state = CREATE;
        static int create_type = ct_node;
        static int select_type = st_single;
        static int adjust_type = at_moving_node;
        auto goto_create = [&](int i){ state = CREATE; create_type = i;  };
        auto goto_select = [&](int i){ state = SELECT; select_type = i;  };
        auto goto_adjust = [&](int i){ state = ADJUST; adjust_type = i;  };

        static ImVec2 a_start{};
        static ImVec2 m_start{};
        static ImVec2 r_start{};
        static unsigned int pin = 0;
        switch ( state )
        {
        case CREATE:{
            if( create_type == ct_node ){
                debug_msg = "create node";
                if( double_clicked_nothing ) {
                    node n = create_node();
                    single_select_node(n);
                    goto_select(st_single);
                }
                if( click_somthing || table_selected){ table_selected = false; goto_select(st_single); }
            }else if( create_type == ct_child ){
                debug_msg = "create child";
                if( !tab_down ){ 
                    node n = create_child(get_selected_node());
                    single_select_node(n);
                    goto_select(st_single);
                }
            }else if( create_type == ct_sibling ){
                debug_msg = "create sibling";
                if(!enter_down){ 
                    node n = create_sibling(get_selected_node());
                    single_select_node(n);
                    goto_select(st_single);
                }
            }else if( create_type == ct_edge ){
                debug_msg = "create edge";
                if( shift_down_valid ){
                    draw_links();
                    if( released_node ){
                        create_edge(get_selected_nodes(), node_hovering);
                    }
                }
                else{
                    goto_select(st_single); 
                }
            }else if( create_type == ct_cluster ){
                debug_msg = "create cluster";
                if( alt_up_valid ){
                    cluster c = create_cluster(get_selected_nodes());
                    unselect_all_node();
                    unselect_all_edge();
                    single_select_cluster(c);
                    goto_select(st_single);
                }
                if( click_somthing ){ goto_select(st_single); }
            }

            if( ctrl_down_valid ){
                goto_select(st_multiple);
            }
            if( dragging_nothing ){
                a_start = mouse_pos;
                goto_select(st_area);
            }
        break;
        }
        case SELECT:{
            if( select_type == st_single ){
                debug_msg = "select single";
                if( released_node ) {
                    single_select_node(node_hovering);
                    unselect_all_edge();
                    unselect_all_cluster();
                }
                else if( released_edge ) {
                    single_select_edge(edge_hovering); 
                    unselect_all_node();
                    unselect_all_cluster();
                }
                else if( released_cluster ) {
                    single_select_cluster(cluster_hovering); 
                    unselect_all_node();
                    unselect_all_edge();
                }
                else if( shift_down_valid && count_selected_node() ) { 
                    goto_create(ct_edge);
                }
                else if( alt_down_valid && count_selected_node() ) { 
                    goto_create(ct_cluster);
                }
                else if( count_selected_node() <= 1 && tab_down && is_window_focused ){
                    goto_create(ct_child);
                }else if ( count_selected_node() <= 1 && enter_down && is_window_focused ){
                    goto_create(ct_sibling);
                }

                if ( dragging_node_pin ){
                    r_start = mouse_pos;
                    pin = pin_hovering;
                    goto_adjust(at_resize_node);
                }
                else if( dragging_node ) {
                    if( count_selected_node() <= 1 || !is_node_focused(node_hovering) && count_selected_node() > 1){
                        single_select_node(node_hovering);
                        unselect_all_edge();
                        unselect_all_cluster();
                    }
                    m_start = mouse_pos;
                    goto_adjust(at_moving_node);
                }
                else if( dragging_cluster ) {
                    m_start = mouse_pos;
                    single_select_cluster(cluster_hovering); 
                    goto_adjust(at_moving_cluster);
                }

                if( ctrl_down_valid ){ goto_select(st_multiple); }
                if( want_delete_valid ){ delete_selected(); }
                if( double_clicked_node ) 
                    { node_properties_popup(node_hovering);}
                else if( double_clicked_edge ) 
                    { edge_properties_popup(edge_hovering);}
                else if( double_clicked_cluster ) 
                    { cluster_properties_popup(cluster_hovering);}
            }
            else if( select_type == st_multiple ){
                debug_msg = "select multiple";
                if( released_node ) {toggle_select_node(node_hovering);}
                else if( released_edge ) { toggle_select_edge(edge_hovering); }
                else if( released_cluster ) { toggle_select_cluster(cluster_hovering); }
                if( want_undo_valid ){ undo_command(); }
                if( want_redo_valid ){ redo_command(); }
                if( want_copy_valid ){ copy_selected(); }
                if( want_paste_valid ){ paste_selected(); }
                if( want_cut_valid ){ cut_selected(); }
                if( want_select_all ) { select_all_node();}
                if( ctrl_up_valid ){ goto_select(st_single);}
            }
            else if( select_type == st_area ){
                debug_msg = "select area";
                if( dragging_valid ) { aera_select_object(a_start, mouse_pos); }
                else {goto_select(st_single);}
            }

            if( click_nothing ){
                unselect_all_node();
                unselect_all_edge();
                unselect_all_cluster();
                goto_create(ct_node);
            }
        break;
        }
        case ADJUST:{
            if( adjust_type == at_moving_node ){
                debug_msg = "adjust moving node";
                if( dragging_valid ){
                    move_node_begin(); 
                    lock_focused_node();
                    search_guidline();
                }
                else{
                    move_node_end(m_start, mouse_pos);
                    goto_select(st_single); 
                    unlock_focused_node();
                    apply_guidline();
                }
            }else if( adjust_type == at_moving_cluster ){
                debug_msg = "adjust moving cluster";
                if( dragging_valid ){
                    unselect_all_node();
                    move_cluster_begin();
                }
                else{
                    unlock_focused_node();
                    unselect_all_node();
                    move_cluster_end(m_start, mouse_pos);
                    goto_select(st_single);
                }
            }else if ( adjust_type == at_resize_node ){
                debug_msg = "resize node";
                if( dragging_valid ){
                    resize_image_begin(pin);
                }
                else{
                    resize_image_end(r_start, mouse_pos, pin);
                    goto_select(st_single);
                }
            }
        break;
        }
        default:
            break;
        }
    };
    void debug(){
        ImGui::Begin("debug");
        ImGui::Text(debug_msg.c_str());
        ImGui::End();
    }
}state_machine_t;

};