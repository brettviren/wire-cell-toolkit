// Per-anode multi-plane DNN-ROI subgraph for ProtoDUNE-VD.
//
// Uses the DNNROIFindingMultiPlane node to feed the two induction planes
// (U+V, stacked along the channel axis) into a single model call, matching
// the per-CRP stacked-induction image the local PDVD training expects
// (1, 4, ~952, 1600 -- W is dropped in training; the fully-convolutional
// MobileNetV3-UNet stretches to the toolkit's runtime channel count).
//
// The PDVD DNN-ROI models are 4-channel.  Input trace tags, in the order
// the model expects (matches the training im_tags
// frame_loose_lf / frame_mp2_roi / frame_mp3_roi / frame_gauss):
//   loose_lf{A}, mp2_roi{A}, mp3_roi{A}, gauss{A}
// NOTE: the 4th channel is gauss -- not PDHD's tight_lf.  Per-channel
// z-scale normalization is baked into the .ts, so input_scale = 1.0.
//
// Every CRP emits DNN-ROI output for both induction planes; the W
// collection plane is passed through from standard SP gauss.
//
// Output: one frame tagged "dnnsp{N}", carrying per-plane trace tags
//   dnnsp{N}u (DNN-ROI on U), dnnsp{N}v (DNN-ROI on V),
//   dnnsp{N}w (standard SP gauss on W, passthrough).
//
// Graph: FrameFanout -> [DNNROIFindingMultiPlane, W PlaneSelector] ->
//        FrameFanin (sets the merged frame tag, keeps the per-plane
//        trace tags).  Mirrors the fanout/fanin shape of dnnroi.jsonnet.

local pg = import 'pgraph.jsonnet';
local wc = import 'wirecell.jsonnet';

function(anode, ts, prefix='dnnroi',
         output_scale=1.0,
         nticks=6000,
         tick_per_slice=4,
         nchunks=1,
         mask_thresh=0.5,
         nchan=4,
         debugfile='')

  local apaid = anode.data.ident;
  local prename = prefix + std.toString(apaid);

  // Input trace tags fed to the model, in the order the model expects.
  // PDVD has only 4-channel models.  Order must match the training im_tags.
  assert nchan == 4 : 'pdvd dnnroi nchan must be 4, got %d' % nchan;
  local apa_intags = [
    'loose_lf%d' % apaid,
    'mp2_roi%d' % apaid,
    'mp3_roi%d' % apaid,
    'gauss%d' % apaid,
  ];

  // The model feeds U+V (stacked) and emits DNN-ROI output for both planes.
  local mp = pg.pnode({
    type: 'DNNROIFindingMultiPlane',
    name: prename,
    data: {
      anode: wc.tn(anode),
      planes: [0, 1],  // feed model U+V (matches training)
      output_planes: [0, 1],
      intags: apa_intags,
      decon_charge_tag: 'decon_charge%d' % apaid,
      outtags: ['dnnsp%du' % apaid, 'dnnsp%dv' % apaid],
      // 4-ch PDVD models bake per-channel z-scale normalization into the
      // .ts, so they run with input_scale 1.0.
      input_scale: 1.0,
      output_scale: output_scale,
      mask_thresh: mask_thresh,
      forward: wc.tn(ts),
      nticks: nticks,
      tick_per_slice: tick_per_slice,
      nchunks: nchunks,
      debugfile: debugfile,
    },
  }, nin=1, nout=1, uses=[ts, anode]);

  // PlaneSelector that pulls gauss%d for the W plane and re-tags it.
  local w_passthrough = pg.pnode({
    type: 'PlaneSelector',
    name: prename + '_wpass',
    data: {
      anode: wc.tn(anode),
      plane: 2,
      tags: ['gauss%d' % apaid],
      tag_rules: [{
        frame: { '.*': 'DNNROIFinding' },
        trace: { ['gauss%d' % apaid]: 'dnnsp%dw' % apaid },
      }],
    },
  }, nin=1, nout=1, uses=[anode]);

  local dnnpipes = [mp, w_passthrough];
  local mult = std.length(dnnpipes);

  local dnnfanout = pg.pnode({
    type: 'FrameFanout',
    name: prename,
    data: { multiplicity: mult },
  }, nin=1, nout=mult);

  // FrameFanin merges the U+V frame and the W frame into one frame tagged
  // 'dnnsp{N}'.  Only the frame tag is rewritten; the per-plane trace tags
  // (dnnsp{N}u / dnnsp{N}v / dnnsp{N}w) set by the nodes are preserved.
  local dnnfanin = pg.pnode({
    type: 'FrameFanin',
    name: prename,
    data: {
      multiplicity: mult,
      tag_rules: [
        { frame: { '.*': 'dnnsp%d' % apaid } }
        for i in std.range(0, mult - 1)
      ],
    },
  }, nin=mult, nout=1);

  pg.intern(
    innodes=[dnnfanout],
    outnodes=[dnnfanin],
    centernodes=dnnpipes,
    edges=[pg.edge(dnnfanout, dnnpipes[i], i, 0) for i in std.range(0, mult - 1)]
          + [pg.edge(dnnpipes[i], dnnfanin, 0, i) for i in std.range(0, mult - 1)],
  )
