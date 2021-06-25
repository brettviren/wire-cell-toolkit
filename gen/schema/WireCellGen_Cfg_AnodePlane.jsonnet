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
    WireCellGen: {Cfg: {AnodePlane: {
    // Construct Face (record)
    // Interesting drift positions on one anode face
    Face(obj=null, anode=0.0, response=0.0, cathode=0.0) :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            anode: $.WireCellUtil.Cfg.Base.Distance(anode),
            response: $.WireCellUtil.Cfg.Base.Distance(response),
            cathode: $.WireCellUtil.Cfg.Base.Distance(cathode),
        },
    }.res,

    // Construct Faces (sequence)
    // One or two faces
    Faces(val) :: assert(std.type(val)=='array'); [$.WireCellGen.Cfg.AnodePlane.Face(v) for v in val],

    // Construct Config (record)
    // Configuration for AnodePlane
    Config(obj=null, ident=0, wire_schema="", nimpacts=10, faces=[]) :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            ident: $.WireCellUtil.Cfg.Base.Count(ident),
            wire_schema: $.WireCellUtil.Cfg.Base.TypeName(wire_schema),
            nimpacts: $.WireCellUtil.Cfg.Base.Count(nimpacts),
            faces: $.WireCellGen.Cfg.AnodePlane.Faces(faces),
        },
    }.res,

     }  }  } 
}