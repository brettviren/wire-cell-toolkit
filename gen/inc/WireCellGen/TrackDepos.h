#ifndef WIRECELL_TRACKDEPOS
#define WIRECELL_TRACKDEPOS

#include "WireCellIface/IDepoSource.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/Units.h"
#include "WireCellAux/Logger.h"

#include "WireCellGen/Cfg/TrackDepos/Structs.hpp"

#include <tuple>
#include <deque>

namespace WireCell {

    namespace Gen {

        /// A producer of depositions created from some number of simple, linear tracks.
        class TrackDepos : public Aux::Logger,
                           public IDepoSource, public IConfigurable {
           public:
            /// Create tracks with depositions every stepsize and assumed
            /// to be traveling at clight.
            TrackDepos(double stepsize = 1.0 * units::millimeter, double clight = 1.0);
            virtual ~TrackDepos();

            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;


            /// ISourceNode
            virtual bool operator()(IDepo::pointer& out);

          private:
            using track_t = WireCellGen::Cfg::TrackDepos::Track;
            using config_t = WireCellGen::Cfg::TrackDepos::Config;
            config_t m_cfg;

            int m_count;
            std::vector<track_t> m_tracks; // keep to enable testing
            std::deque<WireCell::IDepo::pointer> m_depos;

            // WireCell::IDepo::vector depos();
            void add_track(double time, WireCell::Ray ray, double dedx = -1.0);
        };

    }  // namespace Gen
}  // namespace WireCell

#endif
