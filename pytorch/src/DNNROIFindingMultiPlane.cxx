#include "WireCellPytorch/DNNROIFindingMultiPlane.h"
#include "WireCellPytorch/Util.h"

#include <torch/serialize.h>
#include <ATen/core/ivalue.h>
#include <fstream>

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/ITrace.h"

#include "WireCellAux/SimpleFrame.h"
#include "WireCellAux/SimpleTrace.h"
#include "WireCellAux/FrameTools.h"
#include "WireCellAux/PlaneTools.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/TimeKeeper.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

WIRECELL_FACTORY(DNNROIFindingMultiPlane,
                 WireCell::Pytorch::DNNROIFindingMultiPlane,
                 WireCell::IFrameFilter,
                 WireCell::IConfigurable)

using namespace WireCell;

Pytorch::DNNROIFindingMultiPlane::DNNROIFindingMultiPlane()
    : Aux::Logger("DNNROIFindingMultiPlane", "torch")
    , m_save_count(0)
{
}

Pytorch::DNNROIFindingMultiPlane::~DNNROIFindingMultiPlane() {}

void Pytorch::DNNROIFindingMultiPlane::configure(const WireCell::Configuration& cfg)
{
    m_cfg.anode = get(cfg, "anode", m_cfg.anode);

    m_cfg.planes.clear();
    for (const auto& one : cfg["planes"]) {
        m_cfg.planes.push_back(one.asInt());
    }
    if (m_cfg.planes.empty()) {
        log->critical("planes is empty");
        THROW(ValueError() << errmsg{"DNNROIFindingMultiPlane: planes is empty"});
    }

    m_cfg.sort_chanids = get(cfg, "sort_chanids", m_cfg.sort_chanids);

    auto anode = Factory::find_tn<IAnodePlane>(m_cfg.anode);

    m_chlists.clear();
    m_chsets.clear();
    m_row_offsets.clear();
    m_row_offsets.push_back(0);
    for (int plane : m_cfg.planes) {
        auto ichans = Aux::plane_channels(anode, plane);
        std::vector<int> chlist;
        std::unordered_set<int> chset;
        chlist.reserve(ichans.size());
        for (const auto& ichan : ichans) {
            auto chid = ichan->ident();
            chset.insert(chid);
            chlist.push_back(chid);
        }
        if (m_cfg.sort_chanids) {
            std::sort(chlist.begin(), chlist.end());
        }
        log->debug("anode={} plane={} nchans={}",
                   m_cfg.anode, plane, chlist.size());
        m_row_offsets.push_back(m_row_offsets.back() + chlist.size());
        m_chlists.push_back(std::move(chlist));
        m_chsets.push_back(std::move(chset));
    }
    m_total_rows = m_row_offsets.back();

    m_cfg.input_scale = get(cfg, "input_scale", m_cfg.input_scale);
    m_cfg.input_offset = get(cfg, "input_offset", m_cfg.input_offset);
    m_cfg.output_scale = get(cfg, "output_scale", m_cfg.output_scale);
    m_cfg.output_offset = get(cfg, "output_offset", m_cfg.output_offset);

    m_cfg.tick0 = get(cfg, "tick0", m_cfg.tick0);
    m_cfg.nticks = get(cfg, "nticks", m_cfg.nticks);
    m_cfg.mask_thresh = get(cfg, "mask_thresh", m_cfg.mask_thresh);
    m_cfg.forward = get(cfg, "forward", m_cfg.forward);

    m_cfg.intags.clear();
    for (const auto& one : cfg["intags"]) {
        m_cfg.intags.push_back(one.asString());
    }
    if (m_cfg.intags.empty()) {
        log->critical("intags is empty");
        THROW(ValueError() << errmsg{"DNNROIFindingMultiPlane: intags is empty"});
    }

    m_cfg.tick_per_slice = get(cfg, "tick_per_slice", m_cfg.tick_per_slice);
    m_cfg.decon_charge_tag = get(cfg, "decon_charge_tag", m_cfg.decon_charge_tag);
    if (m_cfg.decon_charge_tag.empty()) {
        log->critical("decon_charge_tag is empty");
        THROW(ValueError() << errmsg{"DNNROIFindingMultiPlane: decon_charge_tag is empty"});
    }

    m_cfg.output_planes.clear();
    for (const auto& one : cfg["output_planes"]) {
        m_cfg.output_planes.push_back(one.asInt());
    }
    if (m_cfg.output_planes.empty()) {
        for (size_t i = 0; i < m_cfg.planes.size(); ++i) {
            m_cfg.output_planes.push_back(static_cast<int>(i));
        }
    }
    for (int op : m_cfg.output_planes) {
        if (op < 0 || op >= static_cast<int>(m_cfg.planes.size())) {
            log->critical("output_planes index {} out of range (planes has {} entries)",
                          op, m_cfg.planes.size());
            THROW(ValueError() << errmsg{"DNNROIFindingMultiPlane: output_planes index out of range"});
        }
    }

    m_cfg.outtags.clear();
    for (const auto& one : cfg["outtags"]) {
        m_cfg.outtags.push_back(one.asString());
    }
    if (m_cfg.outtags.size() != m_cfg.output_planes.size()) {
        log->critical("outtags size ({}) != output_planes size ({})",
                      m_cfg.outtags.size(), m_cfg.output_planes.size());
        THROW(ValueError() << errmsg{"DNNROIFindingMultiPlane: outtags must match output_planes"});
    }

    m_cfg.nchunks = get(cfg, "nchunks", m_cfg.nchunks);
    m_cfg.save_negative_charge = get(cfg, "save_negative_charge", m_cfg.save_negative_charge);
    m_cfg.debugfile = get(cfg, "debugfile", m_cfg.debugfile);

    m_ncols = m_cfg.nticks;

    if (m_total_rows == 0) {
        log->critical("empty channel list across all planes");
        THROW(ValueError() << errmsg{"DNNROIFindingMultiPlane: empty channel list"});
    }

    m_forward = Factory::find_tn<ITensorForward>(m_cfg.forward);

    log->info("anode={} planes=[{}] total_rows={} nticks={} tick_per_slice={}",
              m_cfg.anode,
              [&]() {
                  std::string s;
                  for (size_t i = 0; i < m_cfg.planes.size(); ++i) {
                      if (i) s += ",";
                      s += std::to_string(m_cfg.planes[i]);
                  }
                  return s;
              }(),
              m_total_rows, m_cfg.nticks, m_cfg.tick_per_slice);
}

WireCell::Configuration Pytorch::DNNROIFindingMultiPlane::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = m_cfg.anode;
    cfg["planes"] = Json::arrayValue;
    cfg["sort_chanids"] = m_cfg.sort_chanids;
    cfg["input_scale"] = m_cfg.input_scale;
    cfg["input_offset"] = m_cfg.input_offset;
    cfg["output_scale"] = m_cfg.output_scale;
    cfg["output_offset"] = m_cfg.output_offset;
    cfg["tick0"] = m_cfg.tick0;
    cfg["nticks"] = m_cfg.nticks;
    cfg["mask_thresh"] = m_cfg.mask_thresh;
    cfg["forward"] = m_cfg.forward;
    cfg["intags"] = Json::arrayValue;
    cfg["tick_per_slice"] = m_cfg.tick_per_slice;
    cfg["decon_charge_tag"] = m_cfg.decon_charge_tag;
    cfg["output_planes"] = Json::arrayValue;
    cfg["outtags"] = Json::arrayValue;
    cfg["nchunks"] = m_cfg.nchunks;
    cfg["save_negative_charge"] = m_cfg.save_negative_charge;
    cfg["debugfile"] = m_cfg.debugfile;
    return cfg;
}

Array::array_xxf Pytorch::DNNROIFindingMultiPlane::plane_traces_to_eigen(
    size_t plane_index, const ITrace::vector& traces) const
{
    const auto& chset = m_chsets[plane_index];
    const auto& chlist = m_chlists[plane_index];
    Array::array_xxf arr = Array::array_xxf::Zero(chlist.size(), m_ncols);
    ITrace::vector selected;
    selected.reserve(traces.size());
    for (const auto& t : traces) {
        if (chset.find(t->channel()) != chset.end()) {
            selected.push_back(t);
        }
    }
    if (!selected.empty()) {
        Aux::fill(arr, selected,
                  const_cast<std::vector<int>&>(chlist).begin(),
                  const_cast<std::vector<int>&>(chlist).end(),
                  m_cfg.tick0);
    }
    return arr;
}

bool Pytorch::DNNROIFindingMultiPlane::operator()(const IFrame::pointer& inframe,
                                                  IFrame::pointer& outframe)
{
    outframe = inframe;
    if (!inframe) {
        log->debug("EOS at call={}", m_save_count);
        outframe = nullptr;
        return true;
    }

    TimeKeeper tk(fmt::format("call={}", m_save_count));

    // Per-tag, build the stacked (all planes concatenated by row) Eigen
    // array, scale, then downsample ticks.
    std::vector<Array::array_xxf> stacked_per_tag;
    stacked_per_tag.reserve(m_cfg.intags.size());

    for (const auto& tag : m_cfg.intags) {
        auto traces = Aux::tagged_traces(inframe, tag);
        Array::array_xxf big = Array::array_xxf::Zero(m_total_rows, m_ncols);
        for (size_t ip = 0; ip < m_cfg.planes.size(); ++ip) {
            auto a = plane_traces_to_eigen(ip, traces);
            big.block(m_row_offsets[ip], 0, a.rows(), a.cols()) = a;
        }
        if (big.sum() == 0.0) {
            log->warn("call={} no traces for input tag {}, using zeros",
                      m_save_count, tag);
        } else {
            log->debug("call={} tag={} ntraces={}",
                       m_save_count, tag, traces.size());
        }
        big = big * m_cfg.input_scale + m_cfg.input_offset;
        stacked_per_tag.push_back(Array::downsample(big, m_cfg.tick_per_slice, 1));
    }

    // Stack into a (1, ntags, total_rows, ds_ticks) tensor.
    std::vector<torch::Tensor> ch;
    ch.reserve(stacked_per_tag.size());
    for (auto& arr : stacked_per_tag) {
        // from_blob expects (cols, rows) given Eigen column-major data.
        ch.push_back(torch::from_blob(arr.data(), {arr.cols(), arr.rows()}));
    }
    auto img = torch::stack(ch, 0);  // (ntags, ds_ticks, total_rows)
    auto batch = torch::stack({torch::transpose(img, 1, 2)}, 0);  // (1, ntags, total_rows, ds_ticks)

    log->debug(tk(fmt::format("call={} model input shape [{},{},{},{}]",
                              m_save_count,
                              batch.size(0), batch.size(1),
                              batch.size(2), batch.size(3))));

    auto chunks = batch.chunk(m_cfg.nchunks, 2);
    std::vector<torch::Tensor> outputs;
    outputs.reserve(chunks.size());
    for (auto chunk : chunks) {
        std::vector<torch::IValue> itens{(m_cfg.nchunks > 1) ? chunk.clone() : chunk};
        auto iitens = Pytorch::to_itensor(itens);
        auto oitens = m_forward->forward(iitens);
        torch::Tensor ochunk = Pytorch::from_itensor({oitens}).front().toTensor().cpu();
        outputs.push_back(ochunk.clone());
    }
    torch::Tensor output = torch::cat(outputs, 2);
    log->debug(tk(fmt::format("call={} inference done; output shape [{},{},{},{}]",
                              m_save_count,
                              output.size(0), output.size(1),
                              output.size(2), output.size(3))));

    if (!m_cfg.debugfile.empty()) {
        const std::string path = m_cfg.debugfile + "_call"
                                 + std::to_string(m_save_count) + ".pt";
        c10::Dict<std::string, torch::Tensor> meta;
        meta.insert("planes",
                    torch::tensor(std::vector<int64_t>(m_cfg.planes.begin(),
                                                        m_cfg.planes.end())));
        meta.insert("row_offsets",
                    torch::tensor(std::vector<int64_t>(m_row_offsets.begin(),
                                                        m_row_offsets.end())));
        for (size_t ip = 0; ip < m_chlists.size(); ++ip) {
            std::vector<int64_t> ch64(m_chlists[ip].begin(), m_chlists[ip].end());
            meta.insert("chlist_plane" + std::to_string(ip),
                        torch::tensor(ch64));
        }
        meta.insert("tick_per_slice",
                    torch::tensor((int64_t)m_cfg.tick_per_slice));
        meta.insert("input_scale",
                    torch::tensor((double)m_cfg.input_scale));
        meta.insert("mask_thresh",
                    torch::tensor((double)m_cfg.mask_thresh));
        // Pack as a tuple (input, output, meta) and pickle-save so Python's
        // torch.load(path) returns it as a list/tuple.
        std::vector<torch::IValue> tup;
        tup.emplace_back(batch.cpu().clone());
        tup.emplace_back(output.cpu().clone());
        tup.emplace_back(meta);
        torch::IValue root = c10::ivalue::Tuple::create(std::move(tup));
        std::vector<char> bytes = torch::pickle_save(root);
        std::ofstream of(path, std::ios::binary);
        of.write(bytes.data(), bytes.size());
        of.close();
        log->info("call={} debug dump -> {}", m_save_count, path);
    }

    // Tensor -> Eigen (ds_ticks x total_rows), then upsample ticks.
    Eigen::Map<Eigen::ArrayXXf> out_e(output[0][0].data_ptr<float>(),
                                      output.size(3), output.size(2));
    auto mask_e_full = Array::upsample(out_e, m_cfg.tick_per_slice, 0);
    // mask_e_full: (ticks, total_rows)
    log->debug(tk(fmt::format("call={} tensor2eigen", m_save_count)));

    // Gather the decon-charge frame once (it has all planes' channels).
    auto decon_traces = Aux::tagged_traces(inframe, m_cfg.decon_charge_tag);

    // Build output frame: collect all per-plane traces, then tag each
    // plane's range.
    ITrace::vector all_traces;
    std::vector<IFrame::trace_list_t> per_output_indices(m_cfg.output_planes.size());

    // Mark which plane indices we emit, mapping plane_index -> position in
    // output_planes (or -1).
    std::vector<int> emit_pos(m_cfg.planes.size(), -1);
    for (size_t k = 0; k < m_cfg.output_planes.size(); ++k) {
        emit_pos[m_cfg.output_planes[k]] = static_cast<int>(k);
    }

    size_t trace_index = 0;
    for (size_t ip = 0; ip < m_cfg.planes.size(); ++ip) {
        if (emit_pos[ip] < 0) continue;
        const auto& chlist = m_chlists[ip];
        const size_t nrows = chlist.size();
        const size_t roff = m_row_offsets[ip];

        // Slice the mask for this plane: (ticks, nrows)
        Array::array_xxf mask_slice = mask_e_full.block(0, roff, mask_e_full.rows(), nrows);

        // Build this plane's decon-charge array.
        auto decon_arr = plane_traces_to_eigen(ip, decon_traces);
        if (decon_arr.sum() == 0.0) {
            log->warn("call={} no decon-charge traces for plane {} (tag={})",
                      m_save_count, m_cfg.planes[ip], m_cfg.decon_charge_tag);
        }

        // Apply ROI: mask is (ticks, nrows); decon is (nrows, ticks).
        auto sp_T = Array::mask(decon_arr.transpose(), mask_slice, m_cfg.mask_thresh);
        sp_T = Array::baseline_subtraction(sp_T) * m_cfg.output_scale + m_cfg.output_offset;
        Array::array_xxf sp = sp_T.transpose();

        for (size_t irow = 0; irow < nrows; ++irow) {
            ITrace::ChargeSequence charge(m_ncols, 0.0);
            for (size_t icol = 0; icol < m_ncols; ++icol) {
                float v = sp(irow, icol);
                if (!m_cfg.save_negative_charge && v < 0.0f) v = 0.0f;
                charge[icol] = v;
            }
            all_traces.push_back(std::make_shared<Aux::SimpleTrace>(
                chlist[irow], 0, charge));
            per_output_indices[emit_pos[ip]].push_back(trace_index);
            ++trace_index;
        }
    }

    auto traces_sptr = std::make_shared<ITrace::vector>(all_traces.begin(), all_traces.end());
    Aux::SimpleFrame* sframe = new Aux::SimpleFrame(
        inframe->ident(), inframe->time(),
        traces_sptr,
        inframe->tick(), inframe->masks());
    sframe->tag_frame("DNNROIFinding");
    for (size_t k = 0; k < m_cfg.output_planes.size(); ++k) {
        IFrame::trace_summary_t empty_summary;
        sframe->tag_traces(m_cfg.outtags[k], per_output_indices[k], empty_summary);
    }
    outframe = IFrame::pointer(sframe);
    log->debug("call={} output frame: {}", m_save_count, Aux::taginfo(outframe));
    log->debug(tk(fmt::format("call={} finish", m_save_count)));
    ++m_save_count;
    return true;
}
