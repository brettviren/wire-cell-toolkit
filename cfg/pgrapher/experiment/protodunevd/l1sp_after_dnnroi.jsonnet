// Per-anode envelope that wires L1SPFilterPD AFTER a DNN-ROI subgraph,
// feeding the DNN output to L1SP as the "signal" channel.  PDVD port of
// pdhd/l1sp_after_dnnroi.jsonnet — same envelope shape, PDVD-tuned
// L1SPFilterPD config block copied from protodunevd/sp.jsonnet.
//
// Architecture (mirrors the PDHD version):
//
//   post-NF frame (has raw%d) --FrameSplitter--> [port 0] sp -> dnn -> Retagger(dnnsp%d -> gauss%d & wiener%d)
//                                                [port 1] -------------------------- (raw%d preserved) ---+
//                                                  [port 0 = DNN gauss/wiener] -----+                     |
//                                                                                   v                     v
//                                                                              FrameMerger ('replace')
//                                                                              mergemap: gauss, wiener, raw
//                                                                                                |
//                                                                                                v
//                                                                                         L1SPFilterPD
//                                                                                                |
//                                                                                                v
//                                                                                         FrameMerger ('replace')
//                                                                                                |
//                                                                                                v
//                                                                              { raw, gauss (L1SP), wiener (L1SP) }
//
// L1SPFilterPD config below is a verbatim copy of the PDVD block in
// protodunevd/sp.jsonnet:165-243 (per-region kernels, gain_scale,
// gauss_filter suffix, PDVD-tuned trigger-gate overrides and opt-in
// track-veto, peak/mean thresholds).  Keep the two in sync if either is
// re-tuned.

local pg = import 'pgraph.jsonnet';
local wc = import 'wirecell.jsonnet';

function(anode, sp_pipe, dnnroi_pipe, tools, params,
         l1sp_pd_adj_enable=true,
         l1sp_pd_adj_max_hops=3,
         l1sp_pd_planes=null,
         l1sp_pd_dump_mode='process',
         l1sp_pd_dump_path='',
         l1sp_pd_wf_dump_path='',
         l1sp_pd_dump_all_rois=false,
         // ── DNN-mode opts (no PDVD L1SP-DNN model today; plumbing kept
         //    for parity with the PDHD envelope so future calibrations
         //    can drop in a TorchService without rewriting this file).
         l1sp_pd_torch_service=null,
         l1sp_pd_dnn_threshold=0.94,
         l1sp_pd_dnn_window_ticks=256,
         l1sp_pd_dnn_debug_path='')

  local n = anode.data.ident;
  local sfx = if n < 4 then '_b' else '_t';
  local l1sp_planes = if l1sp_pd_planes != null then l1sp_pd_planes else [0, 1];
  // Per-region gain reference: bottom (ident<4) follows params.elec.gain
  // at the 7.8 mV/fC PDVD-bottom reference; top (ident>=4) uses
  // JsonElecResponse and is gain-invariant.  Same convention as
  // protodunevd/sp.jsonnet:165-167.
  local gain_scale = if n < 4
                     then params.elec.gain / (7.8 * wc.mV / wc.fC)
                     else 1.0;
  local kernels_file = if n < 4
                       then 'pdvd_bottom_l1sp_kernels.json.bz2'
                       else 'pdvd_top_l1sp_kernels.json.bz2';

  local pre_split = pg.pnode({
    type: 'FrameSplitter', name: 'predannsplit%d' % n,
  }, nin=1, nout=2);

  // No dnnsp -> gauss relabel here.  protodunevd/dnnroi_mp.jsonnet already
  // emits a final Retagger that merges dnnsp%du/v/w into gauss%d and keeps
  // wiener%d, so the dnnroi_pipe output is already structurally an SP frame.
  local sp_dnn_chain = pg.pipeline(
    [sp_pipe, dnnroi_pipe],
    'sp_dnn_chain_%d' % n);

  local rawsigmerge = pg.pnode({
    type: 'FrameMerger', name: 'dnnrawsigmerge%d' % n,
    data: {
      rule: 'replace',
      mergemap: [
        ['gauss%d'  % n, 'gauss%d'  % n, 'gauss%d'  % n],
        ['wiener%d' % n, 'wiener%d' % n, 'wiener%d' % n],
        ['raw%d'    % n, 'raw%d'    % n, 'raw%d'    % n],
      ],
    },
  }, nin=2, nout=1);

  // L1SPFilterPD — config block copied from protodunevd/sp.jsonnet:178-243.
  local l1sp_node = pg.pnode({
    type: 'L1SPFilterPD',
    name: 'l1sppd_dnn%d' % n,
    data: {
      dft: wc.tn(tools.dft),
      anode: wc.tn(anode),
      kernels_file: kernels_file,
      adctag: 'raw%d' % n,
      sigtag: 'gauss%d' % n,
      outtag: 'gauss%d' % n,
      process_planes: l1sp_planes,
      kernels_scale:       gain_scale,
      l1_raw_asym_eps:     20.0 * gain_scale,
      raw_ROI_th_adclimit: 10.0 * gain_scale,
      adc_sum_threshold:  160.0 * gain_scale,
      gauss_filter: 'HfFilter:Gaus_wide' + sfx,
      l1_adj_enable:  l1sp_pd_adj_enable,
      l1_adj_max_hops: l1sp_pd_adj_max_hops,
      mode: l1sp_pd_dump_mode,   // 'process' | 'dump' | 'dnn'
      dump_mode: l1sp_pd_dump_mode == 'dump',
      dump_path: l1sp_pd_dump_path,
      dump_tag: 'apa%d' % n,
      waveform_dump_path: l1sp_pd_wf_dump_path,
      dump_all_rois: l1sp_pd_dump_all_rois,
      // DNN-mode plumbing — ignored by L1SPFilterPD unless mode == 'dnn'.
      forward: if l1sp_pd_dump_mode == 'dnn' && l1sp_pd_torch_service != null
               then wc.tn(l1sp_pd_torch_service)
               else '',
      dnn_threshold:    l1sp_pd_dnn_threshold,
      dnn_window_ticks: l1sp_pd_dnn_window_ticks,
      dnn_debug_path:   l1sp_pd_dnn_debug_path,
    } + {
      // PDVD-tuned trigger-gate overrides (verbatim from sp.jsonnet:226-242).
      l1_len_long_mod:         180,
      l1_len_fill_shape:       90,
      l1_fill_shape_fill_thr:  0.30,
      l1_fill_shape_fwhm_thr:  0.25,
      l1_pdvd_track_veto_enable: true,
      l1_pdvd_track_high_asym:   0.85,
      l1_pdvd_track_long_cl:     170,
      l1_pdvd_track_med_cl:      100,
      l1_pdvd_track_med_fill:    0.40,
      l1_pdvd_track_med_fwhm:    0.40,
      peak_threshold: 1000,
      mean_threshold: 500,
    },
  }, nin=1, nout=1,
     uses=[tools.dft, anode] +
          (if l1sp_pd_dump_mode == 'dnn' && l1sp_pd_torch_service != null
           then [l1sp_pd_torch_service] else []));

  local final_merger = pg.pnode({
    type: 'FrameMerger', name: 'dnnl1spfinal%d' % n,
    data: {
      rule: 'replace',
      mergemap: [
        ['gauss%d' % n, 'gauss%d'  % n, 'gauss%d'  % n],
        ['gauss%d' % n, 'wiener%d' % n, 'wiener%d' % n],
        ['raw%d'   % n, 'raw%d'    % n, 'raw%d'    % n],
      ],
    },
  }, nin=2, nout=1);

  local post_merge_split = pg.pnode({
    type: 'FrameSplitter', name: 'dnnpostmergesplit%d' % n,
  }, nin=1, nout=2);

  pg.intern(
    innodes=[pre_split],
    centernodes=[sp_dnn_chain, rawsigmerge, post_merge_split, l1sp_node, final_merger],
    edges=[
      pg.edge(pre_split,        sp_dnn_chain,     0, 0),
      pg.edge(sp_dnn_chain,     rawsigmerge,      0, 0),
      pg.edge(pre_split,        rawsigmerge,      1, 1),
      pg.edge(rawsigmerge,      post_merge_split, 0, 0),
      pg.edge(post_merge_split, l1sp_node,        0, 0),
      pg.edge(post_merge_split, final_merger,     1, 1),
      pg.edge(l1sp_node,        final_merger,     0, 0),
    ],
    oports=[final_merger.oports[0]],
    name='dnnroi_l1sp_%d' % n,
  )
