/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2012  Monash University
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * See the file LICENSE.LGPL distributed with the library.
 *
 * Licensees holding a valid commercial license may use this file in
 * accordance with the commercial license agreement provided with the 
 * library.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * Author(s):   Michael Wybrow
*/


#ifndef AVOID_DLLEXPORT_H
#define AVOID_DLLEXPORT_H

#if defined(_MSC_VER) && !defined(LIBAVOID_NO_DLL)
    #ifdef LIBAVOID_EXPORTS
        #define AVOID_EXPORT __declspec(dllexport)
    #else
        #define AVOID_EXPORT __declspec(dllimport)
    #endif
#else
    #define AVOID_EXPORT
#endif

#endif
