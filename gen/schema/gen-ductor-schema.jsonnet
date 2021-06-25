local moo = import "moo.jsonnet";
local wc = import "wirecell.jsonnet";
local util_seq = import "util-base-schema.jsonnet";
local t = moo.oschema.hier(util_seq).WireCellUtil.Cfg.Base;

local wcc = import "cfgschema.jsonnet";
local s = wcc("Gen","Ductor").schema;

// This component's schema needs no local types defined and relies all
// on what util.base provides.  So there is no need for a "working
// object" nor an explicit build().  It's all one (big) statement:

util_seq + [s.component([
        s.field("nsigma", t.Count, default=3.0,
                doc="Depo truncation bound in number of Gaussian standard deviations"),
        s.field("fluctuate", t.Flag, default=true,
                doc="Whether to fluctuate the final Gaussian deposition"),
        s.field("start_time", t.Time, default=0.0,
                doc="The initial time for this ductor"),
        s.field("readout_time", t.Time, default=5*wc.ms,
                doc="The time span for each readout."),
        s.field("tick", t.Tick, default=0.5*wc.us,
                doc="The sampling period"),
        s.field("continuous", t.Flag, default=true,
                doc="Operate in streaming mode"),
        s.field("fixed", t.Flag, default=false,
                doc="Operate in fixed time window mode"),
        s.field("drift_speed", t.Speed,
                doc="The nominal speed of drifting electrons"),
        s.field("first_frame_number", t.Count, default=0,
                doc="Initial value for frame count sequence"),
        s.field("anode", t.TypeName, default="AnodePlane",
                doc="Type name of IAnodePlane component"),
        s.field("rng", t.TypeName, default="Random",
                doc="Type name of IRandom component"),
        s.field("pirs", t.TypeNames, default=[],
                doc="Sequence of type names for IPlaneImpactResponse components"),
], doc="Ductor configuration")]


