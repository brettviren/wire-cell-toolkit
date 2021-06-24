/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains struct and other type definitions for shema in 
 * namespace WireCellUtil::Cfg::Base.
 */
#ifndef WIRECELLUTIL_CFG_BASE_STRUCTS_HPP
#define WIRECELLUTIL_CFG_BASE_STRUCTS_HPP

#include <cstdint>

#include <string>

namespace WireCellUtil::Cfg::Base {

    // @brief A simple counting number
    using Count = int32_t;


    // @brief A spacial distance
    using Distance = double;


    // @brief A code-friendly identifier
    using Ident = std::string;

    // @brief A real number in [0,1]
    using Normalized = double;


    // @brief A Cartesian point in 3-space.
    struct Point {

        // @brief X coordinate
        Distance x = 0.0;

        // @brief Y coordinate
        Distance y = 0.0;

        // @brief Z coordinate
        Distance z = 0.0;
    };

    // @brief A directed line segment in 3-space
    struct Ray {

        // @brief Start point
        Point tail = {};

        // @brief End point
        Point head = {};
    };

    // @brief A temporal duration
    using Time = double;


} // namespace WireCellUtil::Cfg::Base

#endif // WIRECELLUTIL_CFG_BASE_STRUCTS_HPP