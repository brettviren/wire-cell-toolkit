#include "WireCellUtil/PointTree.h"
#include "WireCellUtil/PointTesting.h"
#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

#include "WireCellImg/PointCloudFacade.h"

#include <unordered_map>

using namespace WireCell;
using namespace WireCell::PointTesting;
using namespace WireCell::PointCloud;
using namespace WireCell::PointCloud::Tree;
using namespace WireCell::PointCloud::Facade;
// WireCell::PointCloud::Tree::scoped_pointcloud_t
using spdlog::debug;

using node_ptr = std::unique_ptr<Points::node_t>;

// No more explicit DisjointDataset.  It is a PointCloud::Tree::scoped_pointcloud_t.
template <typename DisjointDataset>
void print_dds(const DisjointDataset& dds) {
    for (size_t idx=0; idx<dds.size(); ++idx) {
        const Dataset& ds = dds[idx];
        std::stringstream ss;
        ss << "ds: " << idx << std::endl;
        // const size_t len = ds.size_major();
        for (const auto& key : ds.keys()) {
            auto arr = ds.get(key)->elements<double>();
            ss << key << ": ";
            for(auto elem : arr) {
                ss << elem << " ";
            }
            ss << std::endl;
        }
        std::cout << ss.str() << std::endl;
    }
}

static
Points::node_ptr make_simple_pctree()
{
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug

    // empty root node
    Points::node_ptr root = std::make_unique<Points::node_t>();

    // Insert a child with a set of named points clouds with one point
    // cloud from a track.

    /// QUESTION: can only do this on construction?
    /// bv: see NaryTree.h for several insert()'s

    /// QUESTION: units?
    /// bv: units are always assumed in WCT system-of-units.  To be correct
    ///     here, we should be multiplying by some [length] unit.

    auto* n1 = root->insert(Points({
        /// QUESTION: proper Array initiation?
        {"scalar", Dataset({
            {"charge", Array({1.0})},
            {"center_x", Array({0.5})},
            {"center_y", Array({0.})},
            {"center_z", Array({0.})},
        })},
        {"3d", make_janky_track(Ray(Point(0, 0, 0), Point(1, 0, 0)))}
        }));

    const Dataset& pc1 = n1->value.local_pcs().at("3d");
    debug("pc1: {}", pc1.size_major());
    

    // Ibid from a different track
    auto* n2 = root->insert(Points({
        {"scalar", Dataset({
            {"charge", Array({2.0})},
            {"center_x", Array({1.5})},
            {"center_y", Array({0.})},
            {"center_z", Array({0.})},
        })},
        {"3d", make_janky_track(Ray(Point(1, 0, 0), Point(2, 0, 0)))}
        }));

    const Dataset& pc2 = n2->value.local_pcs().at("3d");
    debug("pc2: {}", pc2.size_major());

    REQUIRE(pc1 != pc2);
    REQUIRE_FALSE(pc1 == pc2);

    return root;
}

TEST_CASE("test PointTree API")
{
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    auto root = make_simple_pctree();
    CHECK(root.get());

    // from WireCell::NaryTree::Node to WireCell::PointCloud::Tree::Points
    auto& rval = root->value;

    CHECK(root->children().size() == 2);
    CHECK(root.get() == rval.node()); // node raw pointer == point's node
    CHECK(rval.local_pcs().empty());
    
    {
        auto& cval = *(root->child_values().begin());
        const auto& pcs = cval.local_pcs();
        CHECK(pcs.size() > 0);
        // using pointcloud_t = Dataset;
        // key: std::string, val: Dataset
        for (const auto& [key,val] : pcs) {
            debug("child has pc named \"{}\" with {} points", key, val.size_major());
        }
        const auto& pc3d = pcs.at("3d");
        debug("got child PC named \"3d\" with {} points", pc3d.size_major());
        CHECK(pc3d.size_major() > 0);
        CHECK(pc3d.has("x"));
        CHECK(pc3d.has("y"));
        CHECK(pc3d.has("z"));
        CHECK(pc3d.has("q"));
    }

    // name, coords, [depth]
    Scope scope{ "3d", {"x","y","z"}};
    auto const& s3d = rval.scoped_view({ "3d", {"x","y","z"}});

    auto const& pc3d = s3d.pcs();
    CHECK(pc3d.size() == 2);
    print_dds(pc3d);

    auto const& pccenter = rval.scoped_view({ "center", {"x","y","z"}}).pcs();
    print_dds(pccenter);

    const auto& kd = s3d.kd();

    /// QUESTION: how to get it -> node?
    ///
    /// bv: call "index()" on the disjoint range iterator.  This returns a
    /// pair<size_t,size_t> holding major/minor indices into the disjoint range.
    /// You can use that pair to access elements in other disjoint ranges.  See
    /// doctest-pointtree-example for details.

    std::vector<double> some_point = {1, 0, 0};
    auto knn = kd.knn(2, some_point);
    for (auto [it,dist] : knn) {
        auto& pt = *it;
        debug("knn: pt=({},{},{}) dist={}",
              pt[0], pt[1], pt[2], dist);
    }
    CHECK(knn.size() == 2);

    for (size_t pt_ind = 0; pt_ind<knn.size(); ++pt_ind) {
        auto& [pit,dist] = knn[pt_ind];
        const auto [maj_ind,min_ind] = pit.index();
        debug("knn point {} at distance {} from query is in local point cloud {} at index {}",
              pt_ind, dist, maj_ind, min_ind);
        const Dataset& pc = pc3d[maj_ind];
        for (const auto& name : scope.coords) {
            debug("\t{} = {}", name, pc.get(name)->element<double>(min_ind));
        }
    }

    auto rad = kd.radius(.01, some_point);
    for (auto [it,dist] : rad) {
        auto& pt = *it;
        debug("rad: pt=({},{},{}) dist={}",
              pt[0], pt[1], pt[2], dist);
    }
    CHECK(rad.size() == 2);

}


TEST_CASE("test PointCloudFacade")
{
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    auto root = make_simple_pctree();
    Cluster pcc(root);
    auto ave_pos = pcc.calc_ave_pos({1,0,0}, 1);
}