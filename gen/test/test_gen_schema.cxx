#include "WireCellGen/Cfg/Nljs.hpp"
#include "WireCellUtil/nljs2jcpp.hpp"

//using namespace WireCell;
using namespace WireCellGen;

using nljs_t = nlohmann::json;

int main()
{
    Cfg::TrackDepos td;
    nljs_t nljs = td;
    Json::Value jval2 = nljs.get<Json::Value>();
    nljs_t nljs2 = jval2;
    auto td2 = nljs2.get<Cfg::TrackDepos>();

    return 0;
}
