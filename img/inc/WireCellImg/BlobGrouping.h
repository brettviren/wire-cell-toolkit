/** BlobGrouping takes in a channel-level cluster and produces another
 * with channels merged into measure ('m' nodes)
 *
 * The input cluster must have (b-w), (c-w) and (b-s) edges and may have (b-b) edges.
 *
 * The output cluster will add (b-m) and (c-m) edges.
 *
 * The created m-node IMeasures will have sequential ident() unique to
 * the context of the cluster.
 *
 * Grouping is done in the "coarse grained" strategy.  (see raygrid.pdf).
 *
 * See manual for more info.
 */
#ifndef WIRECELL_BLOBGROUPING_H
#define WIRECELL_BLOBGROUPING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell::Img {

    class BlobGrouping : public Aux::Logger, public IClusterFilter, public IConfigurable {
      public:
        BlobGrouping();
        virtual ~BlobGrouping();

        virtual void configure(const WireCell::Configuration& cfg);
        virtual WireCell::Configuration default_configuration() const;

        virtual bool operator()(const input_pointer& in, output_pointer& out);

      private:
        int m_count{0};
    };

}  // namespace WireCell::Img

#endif /* WIRECELL_BLOBGROUPING_H */
