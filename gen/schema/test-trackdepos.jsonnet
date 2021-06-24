local g = import "pgraph.jsonnet";
local wc = import "wirecell.jsonnet";

/// Access the genearted type constructors
local ucb = import "WireCellUtil_Cfg_Base.jsonnet";
local b = ucb.WireCellUtil.Cfg.Base;
local gtd = import "WireCellGen_Cfg_TrackDepos.jsonnet";
local td = gtd.WireCellGen.Cfg.TrackDepos;


// Make use of new schema based configuration.  Note, we can reuse
// some "less safe" constructors such as wc.point() because they
// "accidentally on purpose" produce the same objects.
//
local stubby = b.Ray(tail=wc.point(1000.0, 3.0, 100.0, wc.mm),
                    head=wc.point(1100.0, 3.0, 200.0, wc.mm));
local tracklist = [
    td.Track(time=1*wc.ms, charge=-5000, ray=stubby),
];

local depos = g.pnode({
    type: 'TrackDepos',
    data: td.Config(tracks=tracklist),
}, nin=0, nout=1);
/////


// The rest is "old, manual" configuration style
//
local dump = g.pnode({type:'DumpDepos'}, nin=1, nout=0);

local graph = g.pipeline([depos, dump]);

local app = {
    type: "Pgrapher",
    data: {
        edges: g.edges(graph),
    },
};

local cmdline = {
    type: "wire-cell",
    data: {
        plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellSigProc"],
        apps: ["Pgrapher"]
    }
};

[cmdline] + g.uses(graph) + [app]


