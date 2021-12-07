/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains struct and other type definitions for shema in 
 * namespace WireCellSio::Cfg::NumpyFrameSaver.
 */
#ifndef WIRECELLSIO_CFG_NUMPYFRAMESAVER_STRUCTS_HPP
#define WIRECELLSIO_CFG_NUMPYFRAMESAVER_STRUCTS_HPP

#include <cstdint>
#include "WireCellUtil/Cfg/Base/Structs.hpp"


namespace WireCellSio::Cfg::NumpyFrameSaver {

    // @brief Configuration for NumpyFrameSaver component
    struct Config 
    {

        // @brief If true then truncate to 16 bit short int, o.w. save as 32 bit floats
        WireCellUtil::Cfg::Base::Flag digitize = false;

        // @brief Initial value to which frame samples add
        WireCellUtil::Cfg::Base::Charge baseline = 0.0;

        // @brief Multiplicative scale applied to each frame sample
        WireCellUtil::Cfg::Base::Scaling scale = 1.0;

        // @brief Additive offset applied to scaled frame samples
        WireCellUtil::Cfg::Base::Charge offset = 0.0;

        // @brief Frame tags to consider saving, assume all frames if empty
        WireCellUtil::Cfg::Base::Tags frame_tags = {};

        // @brief Name of NPZ file to which frames are saved
        WireCellUtil::Cfg::Base::FilenameNPZ filename = "wct-frame.npz";
    };

} // namespace WireCellSio::Cfg::NumpyFrameSaver

#endif // WIRECELLSIO_CFG_NUMPYFRAMESAVER_STRUCTS_HPP