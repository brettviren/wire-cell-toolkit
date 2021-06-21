/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains struct and other type definitions for shema in 
 * namespace WireCellGen::Cfg.
 */
#ifndef WIRECELLGEN_CFG_STRUCTS_HPP
#define WIRECELLGEN_CFG_STRUCTS_HPP

#include <cstdint>
#include "WireCellUtil/Cfg/Structs.hpp"

#include <vector>

namespace WireCellGen::Cfg {

    // @brief Amount of charge per some unit
    using Charge = double;


    // @brief 
    struct Track {

        // @brief Absolute time at the start of the track
        WireCellUtil::Cfg::Time time = 0.0;

        // @brief If negative, number of electrons per depo, else electrons per track
        Charge charge = -1.0;

        // @brief The ray defining the track endpoints
        WireCellUtil::Cfg::Ray ray = {};
    };

    // @brief A sequence of tracks
    using Tracks = std::vector<WireCellGen::Cfg::Track>;

    // @brief 
    struct TrackDepos {

        // @brief Distance along track between two neighboring depos.
        WireCellUtil::Cfg::Distance step_size = 1.0;

        // @brief Fraction of speed of light at which track progresses
        WireCellUtil::Cfg::Normalized clight = 1.0;

        // @brief If positive, chunk the depos into groups spaning this amount of time with an EOS delimiting each group.  O.w. all depos are sent out as a stream.
        WireCellUtil::Cfg::Time group_time = -1.0;

        // @brief Description of tracks on which to generate depos.
        Tracks tracks = {};
    };

} // namespace WireCellGen::Cfg

#endif // WIRECELLGEN_CFG_STRUCTS_HPP