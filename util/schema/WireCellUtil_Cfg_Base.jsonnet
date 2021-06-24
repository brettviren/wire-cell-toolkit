/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This holds constructor functions for creating objects from the
 * schema: 
 * 
 * Prefer these functions over manual object construction to assure
 * greater validity.
 */

{
    WireCellUtil: {Cfg: {Base: {
    // Construct Count (number)
    // A simple counting number
    Count(val) :: assert(std.type(val)=='number'); val,

    // Construct Distance (number)
    // A spacial distance
    Distance(val) :: assert(std.type(val)=='number'); val,

    // Construct Ident (string)
    // A code-friendly identifier
    Ident(val) :: assert(std.type(val)=='string'); val,

    // Construct Normalized (number)
    // A real number in [0,1]
    Normalized(val) :: assert(std.type(val)=='number'); val,

    // Construct Point (record)
    // A Cartesian point in 3-space.
    Point(obj=null, x=0.0, y=0.0, z=0.0) :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            x: $.WireCellUtil.Cfg.Base.Distance(x),
            y: $.WireCellUtil.Cfg.Base.Distance(y),
            z: $.WireCellUtil.Cfg.Base.Distance(z),
        },
    }.res,

    // Construct Ray (record)
    // A directed line segment in 3-space
    Ray(obj=null, tail={}, head={}) :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            tail: $.WireCellUtil.Cfg.Base.Point(tail),
            head: $.WireCellUtil.Cfg.Base.Point(head),
        },
    }.res,

    // Construct Time (number)
    // A temporal duration
    Time(val) :: assert(std.type(val)=='number'); val,

     }  }  } 
}