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
#include <imgui.h>
#include <imgui_internal.h>
#include "libcola/cola.h"
#include "libavoid/router.h"
#include "libavoid/libavoid.h"
#include "libavoid/connectionpin.h"

namespace mindmap_layout
{
using namespace ImGui;
using namespace std;
using namespace mindmap;
bool operator==(const vpsc::Rectangle& lhs, const ImRect& rhs) { return fabs(lhs.getCentreX() - rhs.GetCenter().x) < 0.1  && fabs(lhs.getCentreY() - rhs.GetCenter().y) < 0.1;};

struct SetDesiredPos : public cola::PreIteration {
    SetDesiredPos(cola::Locks& locks) : PreIteration(locks) {}
    bool operator()(){
        return should_run;
    }
    void abord(){
        should_run = false;
    }
    bool should_run = true;
};

class LayoutProgress : public cola::TestConvergence
{
    public:
    float stress = DBL_MAX;
    int iterations = 0;
    bool converged = false;
    vector<ImVec2> centers;
    void clear()
    {
        stress = DBL_MAX;
        iterations = 0;
        converged = false;
        centers.clear();
    }
    bool operator()(const double new_stress, valarray<double> & X, valarray<double> & Y)
    {
        converged = TestConvergence::operator()(new_stress,X,Y);
        iterations++;
        stress = new_stress;
        if( converged )
        {
            for (size_t i = 0; i < X.size(); i++)
            {
                ImVec2 c(X[i], Y[i]);
                centers.push_back(c);
            }
        }
        return converged;
    }
};

class RouteProcess : public Avoid::Router
{
    public:
    RouteProcess(Avoid::RouterFlag flag): Router(flag){};
    bool stop = false;
    float progress = 0.0f;
    float total_phases = 0;
    float phase_number = 0;
    double proportion = 0;
    bool  shouldContinueTransactionWithProgress( unsigned int elapsedTime, unsigned int phaseNumber, 
                                                    unsigned int totalPhases, double _proportion){
            proportion = _proportion;
            phase_number = phaseNumber;
            total_phases = totalPhases;
            progress =  phase_number / total_phases;
            return stop;
        }
};

};