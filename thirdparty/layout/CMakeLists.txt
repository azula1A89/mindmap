cmake_minimum_required(VERSION 3.12)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-O3")
add_compile_definitions(NDEBUG)

add_library(vpsc STATIC 
libvpsc/block.cpp
libvpsc/blocks.cpp
libvpsc/constraint.cpp
libvpsc/rectangle.cpp
libvpsc/solve_VPSC.cpp
libvpsc/variable.cpp
libvpsc/cbuffer.cpp
libvpsc/block.h
libvpsc/blocks.h
libvpsc/constraint.h
libvpsc/rectangle.h
libvpsc/pairing_heap.h
libvpsc/solve_VPSC.h
libvpsc/variable.h
libvpsc/cbuffer.h
libvpsc/linesegment.h
libvpsc/assertions.h
)	
target_include_directories(vpsc PUBLIC . libvpsc)

add_library(cola STATIC
libcola/cola.h
libcola/cola.cpp
libcola/colafd.cpp
libcola/conjugate_gradient.cpp
libcola/conjugate_gradient.h
libcola/exceptions.h
libcola/gradient_projection.cpp
libcola/gradient_projection.h
libcola/shortest_paths.h
libcola/straightener.h
libcola/straightener.cpp
libcola/connected_components.cpp
libcola/convex_hull.h
libcola/convex_hull.cpp
libcola/cluster.cpp
libcola/compound_constraints.h
libcola/compound_constraints.cpp
libcola/pseudorandom.h 
libcola/pseudorandom.cpp 
libcola/output_svg.cpp
libcola/output_svg.h 
libcola/unused.h 
libcola/cc_clustercontainmentconstraints.cpp 
libcola/cc_clustercontainmentconstraints.h 
libcola/cc_nonoverlapconstraints.cpp 
libcola/cc_nonoverlapconstraints.h 
libcola/box.cpp 
libcola/box.h 
libcola/shapepair.cpp 
libcola/shapepair.h
)
target_include_directories(cola PUBLIC  . libcola libvpsc)

add_library(avoid STATIC
libavoid/connectionpin.cpp 
libavoid/connector.cpp 
libavoid/connend.cpp 
libavoid/geometry.cpp 
libavoid/geomtypes.cpp 
libavoid/graph.cpp 
libavoid/junction.cpp 
libavoid/makepath.cpp 
libavoid/obstacle.cpp 
libavoid/orthogonal.cpp 
libavoid/router.cpp 
libavoid/shape.cpp 
libavoid/timer.cpp 
libavoid/vertices.cpp 
libavoid/viscluster.cpp 
libavoid/visibility.cpp 
libavoid/vpsc.cpp 
libavoid/hyperedge.cpp 
libavoid/hyperedgeimprover.cpp 
libavoid/mtst.cpp 
libavoid/hyperedgetree.cpp 
libavoid/scanline.cpp 
libavoid/actioninfo.cpp 
libavoid/assertions.h 
libavoid/connector.h 
libavoid/connectionpin.h 
libavoid/connend.h 
libavoid/debug.h 
libavoid/geometry.h 
libavoid/geomtypes.h 
libavoid/graph.h 
libavoid/junction.h 
libavoid/libavoid.h 
libavoid/makepath.h 
libavoid/obstacle.h 
libavoid/orthogonal.h 
libavoid/router.h 
libavoid/shape.h 
libavoid/timer.h 
libavoid/vertices.h 
libavoid/viscluster.h 
libavoid/visibility.h 
libavoid/hyperedge.h 
libavoid/mtst.h 
libavoid/hyperedgetree.h 
libavoid/scanline.h 
libavoid/actioninfo.h 
libavoid/vpsc.h 
libavoid/debughandler.h
)
target_include_directories(avoid PUBLIC . libavoid)



