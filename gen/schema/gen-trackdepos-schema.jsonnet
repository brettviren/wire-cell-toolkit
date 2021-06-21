local wc = import "wirecell.jsonnet";
local moo = import "moo.jsonnet";

// We will use util types
local util_seq = import "util-base-schema.jsonnet";
local t = moo.oschema.hier(util_seq).WireCellUtil.Cfg.Base;

local wcc = import "cfgschema.jsonnet";
local f = wcc("Gen","TrackDepos");
local s = f.schema;

local hier = {
    charge: s.number("Charge", "f8",
                     doc="Amount of charge per some unit"),

    track: s.record("Track", [
        s.field("time", t.Time, 0.0,
                doc="Absolute time at the start of the track"),
        s.field("charge", self.charge, -1.0,
                doc="If negative, number of electrons per depo, else electrons per track"),
        s.field("ray", t.Ray,
                doc="The ray defining the track endpoints"),
    ], doc=""),
    tracks: s.sequence("Tracks", self.track,
                       doc="A sequence of tracks"),

    cfg: s.record("Config", [
        s.field("step_size", t.Distance, 1.0*wc.mm,
                doc="Distance along track between two neighboring depos."),
        s.field("clight", t.Normalized, 1.0,
                doc="Fraction of speed of light at which track progresses"),
        s.field("group_time", t.Time, -1,
                doc="If positive, chunk the depos into groups spaning this amount of time with an EOS delimiting each group.  O.w. all depos are sent out as a stream."),
        s.field("tracks", self.tracks,
                doc="Description of tracks on which to generate depos.")
    ], "Configuration for TrackDepos component")
};
util_seq + f.build(hier)
