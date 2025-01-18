//------------------------------------------------------------------------------
// LICENSE
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
//
// CREDITS
//   Written by Michal Cichon
//------------------------------------------------------------------------------
# ifndef __IMGUI_EXTRA_MATH_H__
# define __IMGUI_EXTRA_MATH_H__
# pragma once


//------------------------------------------------------------------------------
# include <imgui.h>
# ifndef IMGUI_DEFINE_MATH_OPERATORS
// #     define IMGUI_DEFINE_MATH_OPERATORS
# endif
# include <imgui_internal.h>
# include <vector>

//------------------------------------------------------------------------------
struct ImLine
{
    ImVec2 A, B;
};


//------------------------------------------------------------------------------
//  bool operator==(const ImVec2& lhs, const ImVec2& rhs);
//  bool operator!=(const ImVec2& lhs, const ImVec2& rhs);
 ImVec2 operator*(const float lhs, const ImVec2& rhs);
//  ImVec2 operator-(const ImVec2& lhs);


//------------------------------------------------------------------------------
 float  ImLength(float v);
 float  ImLength(const ImVec2& v);
 float  ImLengthSqr(float v);
 ImVec2 ImNormalized(const ImVec2& v);

void calculate_rectangle_contact_point(ImVec2 start, ImVec2 ssize, ImVec2 end, ImVec2& out);
void calculate_line_vertex(ImVec2 start, ImVec2 ssize, ImVec2 end, ImVec2 esize, ImVec2& sout, ImVec2& eout);
void create_triangle_vertex(ImVec2 from, ImVec2 p3, ImVec2& p1, ImVec2& p2, float angle, float arrow_sz);
std::pair<ImVec2, float> get_point_on_polyline(float pos, std::vector<ImVec2> &polyline);
//------------------------------------------------------------------------------
 bool   ImRect_IsEmpty(const ImRect& rect);
 ImVec2 ImRect_ClosestPoint(const ImRect& rect, const ImVec2& p, bool snap_to_edge);
 ImVec2 ImRect_ClosestPoint(const ImRect& rect, const ImVec2& p, bool snap_to_edge, float radius);
 ImVec2 ImRect_ClosestPoint(const ImRect& rect, const ImRect& b);
 ImLine ImRect_ClosestLine(const ImRect& rect_a, const ImRect& rect_b);
 ImLine ImRect_ClosestLine(const ImRect& rect_a, const ImRect& rect_b, float radius_a, float radius_b);



//------------------------------------------------------------------------------
namespace ImEasing {

template <typename V, typename T>
 V EaseOutQuad(V b, V c, T t)
{
    return b - c * (t * (t - 2));
}

} // namespace ImEasing


//------------------------------------------------------------------------------
// # include "imgui_extra_math.inl"


//------------------------------------------------------------------------------
# endif // __IMGUI_EXTRA_MATH_H__
