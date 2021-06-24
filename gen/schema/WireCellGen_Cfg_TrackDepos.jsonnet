/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This holds constructor functions for creating objects from the
 * schema: 
 * 
 * Prefer these functions over manual object construction to assure
 * greater validity.
 */
local WireCellUtil_Cfg_Base = import "WireCellUtil_Cfg_Base.jsonnet";

WireCellUtil_Cfg_Base +
{
    WireCellGen: {Cfg: {TrackDepos: {
    // Construct Charge (number)
    // Amount of charge per some unit
    Charge(val) :: assert(std.type(val)=='number'); val,

    // Construct Track (record)
    // 
    Track(obj=null, time=0.0, charge=-1.0, ray={}) :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            time: $.WireCellUtil.Cfg.Base.Time(time),
            charge: $.WireCellGen.Cfg.TrackDepos.Charge(charge),
            ray: $.WireCellUtil.Cfg.Base.Ray(ray),
        },
    }.res,

    // Construct Tracks (sequence)
    // A sequence of tracks
    Tracks(val) :: assert(std.type(val)=='array'); [$.WireCellGen.Cfg.TrackDepos.Track(v) for v in val],

    // Construct Config (record)
    // Configuration for TrackDepos component
    Config(obj=null, step_size=1.0, clight=1.0, group_time=-1.0, tracks=[]) :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            step_size: $.WireCellUtil.Cfg.Base.Distance(step_size),
            clight: $.WireCellUtil.Cfg.Base.Normalized(clight),
            group_time: $.WireCellUtil.Cfg.Base.Time(group_time),
            tracks: $.WireCellGen.Cfg.TrackDepos.Tracks(tracks),
        },
    }.res,

     }  }  } 
}