local wcc = import "cfgschema.jsonnet";

local moo = import "moo.jsonnet";
local f = wcc("Util", "Base");
local s = f.schema;

local hier = {
    ident: s.string("Ident", pattern=moo.re.ident_only,
                    doc="A code-friendly identifier"),

    count: s.number("Count", "i4",
                    doc="A simple counting number"),

    distance: s.number("Distance", "f8",
                       doc="A spacial distance"),
    time: s.number("Time", "f8",
                   doc="A temporal duration"),
    
    normalized: s.number("Normalized", "f8",
                         s.nc(minimum=0.0, maximum=1.0),
                         doc="A real number in [0,1]"),
    point: s.record("Point", [
        s.field("x", self.distance, doc="X coordinate"),
        s.field("y", self.distance, doc="Y coordinate"),
        s.field("z", self.distance, doc="Z coordinate"),
    ], doc="A Cartesian point in 3-space."),

    ray: s.record("Ray", [
        s.field("tail", self.point, doc="Start point"),
        s.field("head", self.point, doc="End point"),
    ], doc="A directed line segment in 3-space"),
};
f.build(hier)
