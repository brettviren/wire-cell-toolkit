/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This holds constructor functions for creating objects from the
 * schema: sio.NumpyFrameSaver
 * 
 * Prefer these functions over manual object construction to assure
 * greater validity.
 *
 */
local util_Base = import "schema/util/Base.jsonnet";

util_Base +
{
    sio: { NumpyFrameSaver: {
    // Construct Config (record)
    // Configuration for NumpyFrameSaver component
    Config(obj=null, digitize=false, baseline=0.0, scale=1.0, offset=0.0, frame_tags=[], filename="wct-frame.npz") :: {
        assert(std.setMember(std.type(obj), ["null", "object"])),
        res: if std.type(obj) == 'object' then obj else {
            digitize: $.util.Base.Flag(digitize),
            baseline: $.util.Base.Charge(baseline),
            scale: $.util.Base.Scaling(scale),
            offset: $.util.Base.Charge(offset),
            frame_tags: $.util.Base.Tags(frame_tags),
            filename: $.util.Base.FilenameNPZ(filename),
        },
    }.res,

    } }, // sio.NumpyFrameSaver
}
