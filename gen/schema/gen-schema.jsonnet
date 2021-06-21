// pip install git+https://github.com/brettviren/moo.git
// moo compile ...
local moo = import "moo.jsonnet";

// We will use util types
local util_seq = import "util-schema.jsonnet";
local util_hier = moo.oschema.hier(util_seq);


// In wire-cell-toolkit/cfg/
local wcc = import "cfgschema.jsonnet";

local f = wcc("Gen");
local s = wcc.schema;

local td = import "trackdepos.jsonnet";
local hier = td(f.schema, util_hier.WireCellUtil.Cfg);

util_seq + f.build(hier)
//hier
