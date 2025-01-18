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

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <future>
#include <chrono>
#include <array>
#include <algorithm>
#include <utility>
#include <memory>
#include <math.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <imgui.h>
#include <IconsFontAwesome6.h>
#include <IconsMaterialDesign.h>
#include <sstream>
#include <regex>


struct util
{
    static std::size_t replace_all(std::string& inout, std::string_view what, std::string_view with)
    {
        std::size_t count{};
        for (std::string::size_type pos{};
            inout.npos != (pos = inout.find(what.data(), pos, what.length()));
            pos += with.length(), ++count) {
            inout.replace(pos, what.length(), with.data(), with.length());
        }
        return count;
    }
    static void lower_case(std::string& inout){
        for(auto& i: inout){
            i = std::tolower(i);
        }
    }
    static uint32_t reverse_bytes(uint32_t x) {
        x = ((x & 0x0000FFFF) << 16) | ((x & 0xFFFF0000) >> 16);
        x = ((x & 0x00FF00FF) << 8)  | ((x & 0xFF00FF00) >> 8);
        return x;
    }
    static std::string ImU32toHex(ImU32 col)
    {
        return fmt::format("#{:08X}", reverse_bytes(col));
    }
    static ImU32 HexToImU32(std::string_view hex)
    {
        ImU32 col;
        if (sscanf(hex.data(), "#%08X", &col) != 1) {
            throw std::invalid_argument("Invalid hex string");
        }
        return reverse_bytes(col);
    }
    static void wrap_text(std::string& str, float width){
        if( str.size() < width ) return;
        size_t n = std::ceil(str.size() / width);
        for (size_t i = 0; i < n; i++) {
            str.insert(i*width, "\n");
        }
    }
    static std::vector<std::string> split(std::string& str, std::string_view delimiter){
        size_t pos = 0;
        std::string token;
        std::vector<std::string> tokens;
        while ((pos = str.find(delimiter)) != std::string::npos) {
            token = str.substr(0, pos);
            tokens.push_back(token);
            str.erase(0, pos + delimiter.length());
        }
        tokens.push_back(str);
        return tokens;
    }
    static void wrap_word(std::string& str, unsigned int width){
        auto words = split(str, " ");
        int line_length = 0;
        std::string result = "";
        for(auto it = words.begin(); it != words.end(); ++it) {
            if( line_length + it->size() > width ) {
                result.append("\n");
                line_length = it->size();
            }else {
                if( line_length ){  result.append(" "); line_length++; }
                line_length += it->size();
            }
            result.append(*it);
        }
        str = result;
    }
    static bool is_valid_url(const std::string& url) {
        // std::regex pattern(R"(^(https?://)?(www\.)?[-a-z0-9]+(\.[-a-z0-9]+)+(:\d+)?(/[-a-z0-9_\/\?&=#.]*)?$)");
        return !url.empty() ;//&& std::regex_match(url, pattern);
    }
    static ImVec2 random_point(ImVec2 center, float random_radius) {
        float angle = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
        float radius = static_cast<float>(rand()) / RAND_MAX * random_radius;
        float x = center.x + radius * cos(angle);
        float y = center.y + radius * sin(angle);
        return ImVec2(x, y);
    }
};


typedef struct item_s
{
    int m_id;
    std::string m_label;
    int m_indeg;
    int m_outdeg;
    int m_deg;
    bool m_locked;
    std::string m_status;
    item_s():m_id(0), m_label(""),m_indeg(0),m_outdeg(0),m_deg(0),m_locked(false),m_status(""){};
    item_s(int id, std::string label, int indeg, int outdeg, bool lock, std::string status):
        m_id(id), m_label(label),m_indeg(indeg),m_outdeg(outdeg),m_deg(m_indeg+m_outdeg), m_locked(lock),m_status(status){};
    bool operator==(const item_s& rhs){ return (m_id == rhs.m_id); };
}item_t;

typedef enum class situation_e{ v_align_ll = 1<<0|1<<6,
                                v_align_lc = 1<<0|1<<7,
                                v_align_lr = 1<<0|1<<8,
                                v_align_cl = 1<<1|1<<6,
                                v_align_cc = 1<<1|1<<7,
                                v_align_cr = 1<<1|1<<8,
                                v_align_rl = 1<<2|1<<6,
                                v_align_rc = 1<<2|1<<7,
                                v_align_rr = 1<<2|1<<8,
                                h_align_tt = 1<<3|1<<9,
                                h_align_tc = 1<<3|1<<10,
                                h_align_tb = 1<<3|1<<11,
                                h_align_ct = 1<<4|1<<9,
                                h_align_cc = 1<<4|1<<10,
                                h_align_cb = 1<<4|1<<11,
                                h_align_bt = 1<<5|1<<9,
                                h_align_bc = 1<<5|1<<10,
                                h_align_bb = 1<<5|1<<11,
                                }situation_t;
typedef struct alignment_s
{
    int m_id1;
    int m_id2;
    std::vector<situation_t> m_situations;
    std::vector<std::pair<ImVec2,ImVec2>> guidlines;
    std::vector<ImVec2> guidpoints;
    alignment_s():m_id1(0), m_id2(0){};
    alignment_s(int id1, int id2, std::vector<situation_t> situations):m_id1(id1), m_id2(id2), m_situations(situations){};
}alignment_t;
typedef std::vector<alignment_t> alignment_list_t;
enum cola_alignment{ align_right, align_center, align_left };

void draw();


