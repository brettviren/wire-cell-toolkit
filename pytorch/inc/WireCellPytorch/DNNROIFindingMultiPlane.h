/** Multi-plane variant of DNNROIFinding.
 *
 * Stacks two or more wire planes along the channel axis into a single
 * model call, then splits the per-pixel ROI mask back into per-plane
 * outputs.  Useful when a model is trained on multiple planes
 * concatenated (e.g. U+V on PDHD with input shape 1600x1500), where
 * the per-plane DNNROIFinding node does not match.
 *
 * The output frame carries one trace tag per plane (taken from the
 * `outtags` config list, in the same order as `planes`).
 */

#ifndef WIRECELLPYTORCH_DNNROIFINDINGMULTIPLANE
#define WIRECELLPYTORCH_DNNROIFINDINGMULTIPLANE

#include "WireCellIface/ITensorForward.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameFilter.h"
#include "WireCellUtil/Array.h"
#include "WireCellAux/Logger.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace WireCell {
    namespace Pytorch {

        struct DNNROIFindingMultiPlaneCfg {
            std::string anode{"AnodePlane"};

            // Plane indices (0=U, 1=V, 2=W) to stack along the channel
            // axis in the order given.  Must contain at least one plane.
            std::vector<int> planes;

            // Subset of planes (by index into `planes`, NOT by plane
            // number) whose model output should be emitted as a tagged
            // trace set.  Empty means "all planes in `planes`".  Use
            // this to suppress a plane's DNN-ROI output while still
            // feeding it to the model (e.g. APA0 V plane on PDHD).
            std::vector<int> output_planes;

            bool sort_chanids{false};

            double input_scale{1.0 / 4000};
            double input_offset{0.0};
            double output_scale{1.0};
            double output_offset{0.0};

            int tick0{0};
            int nticks{6000};

            double mask_thresh{0.5};

            std::string forward{"TorchService"};

            // Three trace tags (typically loose_lfN / mp2_roiN / mp3_roiN)
            std::vector<std::string> intags;

            std::string decon_charge_tag{""};

            int tick_per_slice{4};

            // Output trace tag per plane, same length as planes.
            std::vector<std::string> outtags;

            int nchunks{1};

            // Round model_ticks up to a multiple of this value before
            // running inference.  Use 4 (== tick_per_slice) when the
            // model can absorb any post-rebin width.  Set larger (e.g.
            // 128 for PDVD's traced NestedUNet, which needs the post-
            // rebin width divisible by 32 to survive the U-Net's 5
            // stride-2 levels evenly).  When 0 or unset, defaults to
            // tick_per_slice — current pre-knob behaviour.
            int tick_pad_multiple{0};

            bool save_negative_charge{false};

            // If non-empty, after each call write
            //   {debugfile}_call{N}.pt
            // containing the model's (input, output) tensors plus a
            // metadata dict (plane indices, per-plane row offsets and
            // channel-id lists, tick_per_slice, input_scale,
            // mask_thresh, anode ident).  Loadable in Python with
            //   torch.jit.load(path).attr(0) / torch.load(path).
            // Off by default; introduces no runtime cost when empty.
            std::string debugfile{""};
        };

        class DNNROIFindingMultiPlane : public Aux::Logger,
                                        public IFrameFilter,
                                        public IConfigurable {
          public:
            DNNROIFindingMultiPlane();
            virtual ~DNNROIFindingMultiPlane();

            virtual bool operator()(const IFrame::pointer& inframe,
                                    IFrame::pointer& outframe);

            virtual WireCell::Configuration default_configuration() const;
            virtual void configure(const WireCell::Configuration& config);

          private:
            DNNROIFindingMultiPlaneCfg m_cfg;

            // Per-plane channel-ID lists, in the order given by planes.
            std::vector<std::vector<int>> m_chlists;
            // Per-plane set for fast membership.
            std::vector<std::unordered_set<int>> m_chsets;
            // Row offset (cumulative #channels) where each plane starts
            // in the stacked channel axis.  size = planes.size() + 1.
            std::vector<size_t> m_row_offsets;

            size_t m_total_rows{0};   // sum of all planes' channels
            size_t m_ncols{0};        // == m_cfg.nticks

            ITensorForward::pointer m_forward{nullptr};

            int m_save_count;

            // Build an Eigen array for one plane filling its channels
            // from the given trace vector.  Output rows = nchans of that
            // plane, cols = ntick.
            Array::array_xxf plane_traces_to_eigen(size_t plane_index,
                                                   const ITrace::vector& traces,
                                                   int ntick) const;
        };
    }  // namespace Pytorch
}  // namespace WireCell

#endif
