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
    WireCellGen: {Cfg: {Ductor: {
    // Construct Config (record)
    // Ductor configuration
    Config(obj=null, nsigma=3, fluctuate=true, start_time=0.0, readout_time=5000000.0, tick=500.0, continuous=true, fixed=false, drift_speed=0.0, first_frame_number=0, anode="AnodePlane", rng="Random", pirs=[]) :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            nsigma: $.WireCellUtil.Cfg.Base.Count(nsigma),
            fluctuate: $.WireCellUtil.Cfg.Base.Flag(fluctuate),
            start_time: $.WireCellUtil.Cfg.Base.Time(start_time),
            readout_time: $.WireCellUtil.Cfg.Base.Time(readout_time),
            tick: $.WireCellUtil.Cfg.Base.Tick(tick),
            continuous: $.WireCellUtil.Cfg.Base.Flag(continuous),
            fixed: $.WireCellUtil.Cfg.Base.Flag(fixed),
            drift_speed: $.WireCellUtil.Cfg.Base.Speed(drift_speed),
            first_frame_number: $.WireCellUtil.Cfg.Base.Count(first_frame_number),
            anode: $.WireCellUtil.Cfg.Base.TypeName(anode),
            rng: $.WireCellUtil.Cfg.Base.TypeName(rng),
            pirs: $.WireCellUtil.Cfg.Base.TypeNames(pirs),
        },
    }.res,

     }  }  } 
}