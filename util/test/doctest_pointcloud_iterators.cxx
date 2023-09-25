#include "WireCellUtil/PointCloudIterators.h"
#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/doctest.h"


#include <vector>

using spdlog::debug;
using namespace WireCell;
using namespace WireCell::PointCloud;

TEST_CASE("point cloud iterate vector selection")
{
    Dataset ds({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})},
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})}});
    auto sel = ds.selection({"x","y","z"});

    debug("doctest_pointcloud_iterator: make iterators");
    coordinate_iterator<std::vector<double>> cit(sel), end;

    CHECK(cit.size() == 3); 
    CHECK(cit.ndim() == 3); 
    CHECK(std::distance(cit,end) == 3);
    CHECK(std::distance(cit,cit) == 0);
    CHECK(std::distance(end,end) == 0);

    debug("doctest_pointcloud_iterator: copy iterator");
    coordinate_iterator<std::vector<double>> cit2 = cit;

    CHECK(cit2.size() == 3); 
    CHECK(cit2.ndim() == 3); 

    debug("doctest_pointcloud_iterator: iterate");

    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 2.0);
    CHECK((*cit)[2] == 1.0);

    ++cit;                        // column 2

    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 1.0);
    CHECK((*cit)[2] == 4.0);

    ++cit;                        // column 3

    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 3.0);
    CHECK((*cit)[2] == 1.0);

    ++cit;                        // end

    CHECK (cit == end);

    --cit;                      // back to column 3

    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 3.0);
    CHECK((*cit)[2] == 1.0);

    cit -= 2;                   // back to start
    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 2.0);
    CHECK((*cit)[2] == 1.0);

    for (auto p : coordinates<Point>(sel, Point())) {
        debug("x={} y={} z={}", p.x(), p.y(), p.z());
    }
}
