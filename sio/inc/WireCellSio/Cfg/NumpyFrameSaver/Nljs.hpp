/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains functions struct and other type definitions for shema in 
 * namespace WireCellSio::Cfg::NumpyFrameSaver to be serialized via nlohmann::json.
 */
#ifndef WIRECELLSIO_CFG_NUMPYFRAMESAVER_NLJS_HPP
#define WIRECELLSIO_CFG_NUMPYFRAMESAVER_NLJS_HPP

// My structs
#include "WireCellSio/Cfg/NumpyFrameSaver/Structs.hpp"

// Nljs for externally referenced schema
#include "WireCellUtil/Cfg/Base/Nljs.hpp"

#include <nlohmann/json.hpp>

namespace WireCellSio::Cfg::NumpyFrameSaver {

    using data_t = nlohmann::json;
    
    inline void to_json(data_t& j, const Config& obj) {
        j["digitize"] = obj.digitize;
        j["baseline"] = obj.baseline;
        j["scale"] = obj.scale;
        j["offset"] = obj.offset;
        j["frame_tags"] = obj.frame_tags;
        j["filename"] = obj.filename;
    }
    
    inline void from_json(const data_t& j, Config& obj) {
        if (j.contains("digitize"))
            j.at("digitize").get_to(obj.digitize);    
        if (j.contains("baseline"))
            j.at("baseline").get_to(obj.baseline);    
        if (j.contains("scale"))
            j.at("scale").get_to(obj.scale);    
        if (j.contains("offset"))
            j.at("offset").get_to(obj.offset);    
        if (j.contains("frame_tags"))
            j.at("frame_tags").get_to(obj.frame_tags);    
        if (j.contains("filename"))
            j.at("filename").get_to(obj.filename);    
    }
    
} // namespace WireCellSio::Cfg::NumpyFrameSaver

#endif // WIRECELLSIO_CFG_NUMPYFRAMESAVER_NLJS_HPP