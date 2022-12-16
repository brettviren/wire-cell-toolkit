#include "WireCellUtil/WireSchema.h"
#include "WireCellUtil/Persist.h"
#include "WireCellUtil/Configuration.h"

using namespace WireCell;
using namespace WireCell::WireSchema;

static
void load_file(const std::string& path, StoreDB& store)
{
    Json::Value jtop = WireCell::Persist::load(path);
    Json::Value jstore = jtop["Store"];

    std::vector<Point> points;
    {
        Json::Value jpoints = jstore["points"];
        const int npoints = jpoints.size();
        points.resize(npoints);
        for (int ipoint = 0; ipoint < npoints; ++ipoint) {
            Json::Value jp = jpoints[ipoint]["Point"];
            points[ipoint].set(get<double>(jp, "x"), get<double>(jp, "y"), get<double>(jp, "z"));
        }
    }

    {  // wires
        Json::Value jwires = jstore["wires"];
        const int nwires = jwires.size();
        std::vector<Wire>& wires = store.wires;
        wires.resize(nwires);
        for (int iwire = 0; iwire < nwires; ++iwire) {
            Json::Value jwire = jwires[iwire]["Wire"];
            Wire& wire = wires[iwire];
            wire.ident = get<int>(jwire, "ident");
            wire.channel = get<int>(jwire, "channel");
            wire.segment = get<int>(jwire, "segment");

            int itail = get<int>(jwire, "tail");
            int ihead = get<int>(jwire, "head");
            wire.tail = points[itail];
            wire.head = points[ihead];
        }
    }

    {  // planes
        Json::Value jplanes = jstore["planes"];
        const int nplanes = jplanes.size();
        std::vector<Plane>& planes = store.planes;
        planes.resize(nplanes);
        for (int iplane = 0; iplane < nplanes; ++iplane) {
            Json::Value jplane = jplanes[iplane]["Plane"];
            Plane& plane = planes[iplane];
            plane.ident = get<int>(jplane, "ident");
            Json::Value jwires = jplane["wires"];
            const int nwires = jwires.size();
            plane.wires.resize(nwires);
            for (int iwire = 0; iwire < nwires; ++iwire) {
                plane.wires[iwire] = convert<int>(jwires[iwire]);
            }
        }
    }

    {  // faces
        Json::Value jfaces = jstore["faces"];
        const int nfaces = jfaces.size();
        std::vector<Face>& faces = store.faces;
        faces.resize(nfaces);
        for (int iface = 0; iface < nfaces; ++iface) {
            Json::Value jface = jfaces[iface]["Face"];
            Face& face = faces[iface];
            face.ident = get<int>(jface, "ident");
            Json::Value jplanes = jface["planes"];
            const int nplanes = jplanes.size();
            face.planes.resize(nplanes);
            for (int iplane = 0; iplane < nplanes; ++iplane) {
                face.planes[iplane] = convert<int>(jplanes[iplane]);
            }
        }
    }

    {  // anodes
        Json::Value janodes = jstore["anodes"];
        const int nanodes = janodes.size();
        std::vector<Anode>& anodes = store.anodes;
        anodes.resize(nanodes);
        for (int ianode = 0; ianode < nanodes; ++ianode) {
            Json::Value janode = janodes[ianode]["Anode"];
            Anode& anode = anodes[ianode];
            anode.ident = get<int>(janode, "ident");
            Json::Value jfaces = janode["faces"];
            const int nfaces = jfaces.size();
            anode.faces.resize(nfaces);
            for (int iface = 0; iface < nfaces; ++iface) {
                anode.faces[iface] = convert<int>(jfaces[iface]);
            }
        }
    }

    {  // detectors
        std::vector<Detector>& dets = store.detectors;

        Json::Value jdets = jstore["detectors"];
        const int ndets = jdets.size();

        if (!ndets) {           // some do not give us this section
            dets.resize(1);
            auto& det = dets[0];
            det.ident = 0;
            int nanodes = store.anodes.size();
            det.anodes.resize(nanodes);
            for (int ianode = 0; ianode < nanodes; ++ianode) {
                det.anodes[ianode] = ianode;
            }
        }
        else {
            dets.resize(ndets);
            for (int idet = 0; idet < ndets; ++idet) {
                Json::Value jdet = jdets[idet]["Detector"];
                Detector& det = dets[idet];
                det.ident = get<int>(jdet, "ident");
                Json::Value janodes = jdet["anodes"];
                const int nanodes = janodes.size();
                det.anodes.resize(nanodes);
                for (int ianode = 0; ianode < nanodes; ++ianode) {
                    det.anodes[ianode] = convert<int>(janodes[ianode]);
                }
            }
        }
    }
}

// Return axis along which wire centers are ascending when in proper
// wire-in-plane order.
static int wire_order_axis(const StoreDB& store, const Plane& plane)
{
    const auto& w = store.wires[plane.wires[0]];
    Vector wdir = ray_unit(Ray(w.tail, w.head));
    if (std::abs(wdir.z()) > 0.9999) { // less than ~1 deg from Z
        return 1;                      // then sort by Y
    }
    return 2;                   // sort by Z
}

// Sort wires according to their center coordinates
struct wip_order {
    const StoreDB &store;
    size_t axis{2};

    bool operator()(int a, int b) const {
        const auto& wa = store.wires[a];
        const auto& wb = store.wires[b];        
        return wa.head[axis] + wa.tail[axis] < wb.head[axis] + wb.tail[axis];
    }
};

using plane_fixer_f = std::function<void(StoreDB& store, Plane& plane)>;
using plane_fixers_t = std::vector<plane_fixer_f>;

// Fix order of plane's wires-in-plane and wire endpoints.
static void plane_fixer_order(StoreDB& store, Plane& plane)
{    
    const size_t axis = wire_order_axis(store, plane);

    // Wire-in-plane ordering
    std::sort(plane.wires.begin(), plane.wires.end(),
              wip_order{store, axis});

    // endoint ordering
    for (int iwire : plane.wires) {
        Wire& wire = store.wires[iwire];

        if (axis == 1) {
            if (wire.head.z() > wire.tail.z()) {
                std::swap(wire.head, wire.tail);
            }
        }
        else {
            if (wire.head.y() < wire.tail.y()) {
                std::swap(wire.head, wire.tail);
            }
        }
    }
}

// Fix wire directions.  Assumes wire endpoint order is correct.
static void plane_fixer_direction(StoreDB& store, Plane& plane)
{
    Vector wdir;
    const size_t nwires = plane.wires.size();
    std::vector<double> half(nwires);

    for (size_t wind=0; wind<nwires; ++wind) {
        const Wire wire = store.wires[plane.wires[wind]];
        const Ray ray(wire.tail, wire.head);
        const auto rv = ray_vector(ray);
        wdir += rv;
        half[wind] = 0.5*rv.magnitude();
    }

    wdir.x(0);                  // bring into Y-Z plane
    wdir = wdir.norm();

    // Correct wire endpoints.
    for (size_t wind=0; wind<nwires; ++wind) {
        auto& wire = store.wires[plane.wires[wind]];
        const auto c = 0.5*(wire.tail + wire.head);
        const Vector whalf = wdir * half[wind];
        wire.head = c + whalf;
        wire.tail = c - whalf;
    }        
}

// uniform pitch, coplanar wires.  Assumes direction is correcterd
static void plane_fixer_pitch(StoreDB& store, Plane& plane)
{
    // Find the mean X position of wire centers
    double xmean = 0;
    const size_t nwires = plane.wires.size();
    for (size_t wind=0; wind<nwires; ++wind) {
        const auto& wire = store.wires[plane.wires[wind]];
        xmean += 0.5*(wire.tail.x() + wire.head.x());
    }
    xmean /= nwires;

    // mean pitch.  average all pitches.  Not sure best strategy.
    // Imprecise endoints lead to less precision on short wires but
    // including short wires means averaging over more.  Do simplest
    // thing and average over all of them.

    const size_t nhalf = nwires/2;
    Ray midway;                 // pick up central wire 

    Vector ptot;
    Ray prev;

    for (size_t wind=0; wind<nwires; ++wind) {
        auto& wire = store.wires[plane.wires[wind]];
        wire.tail.x(xmean);     // move wire to
        wire.head.x(xmean);     // average X location
        Ray next(wire.tail, wire.head);
        if (wind == nhalf) {
            midway = next;
        }
        if (wind) {            // wait until 2nd to calculate diff
            ptot += ray_vector(ray_pitch(prev, next));
        }
        prev = next;
    }
    const Vector pmean = ptot / (nwires-1);

    for (size_t wind=0; wind<nwires; ++wind) {
        if (wind == nhalf) {
            // dont' correct midway
            continue;
        }
        auto& wire = store.wires[plane.wires[wind]];
        // have this pitch from midway to this wire.
        const auto have = ray_vector(ray_pitch(midway, Ray(wire.tail, wire.head)));
        // want this pitch from N steps from midway
        const auto want = ((int)wind - (int)nhalf) * pmean;
        // difference
        const auto diff = want - have;

        wire.tail += diff;
        wire.head += diff;
    }

}

static void fix_planes(StoreDB& store, plane_fixers_t& fixers)
{
    if (fixers.empty()) return;

    for (auto& detector : store.detectors) {
        for (int ianode : detector.anodes) {
            auto& anode = store.anodes[ianode];
            for (int iface : anode.faces) {
                auto& face = store.faces[iface];
                for (int iplane : face.planes) {
                    auto& plane = store.planes[iplane];

                    for (auto& fixer : fixers) {
                        fixer(store, plane);
                    }
                }
            }
        }
    }
}



Store WireCell::WireSchema::load(const char* filename, Correction correction)
{
    using cache_key_t = std::pair<std::string, WireSchema::Correction>;
    static std::map<cache_key_t, StoreDBPtr> cache;

    // turn into absolute real path
    std::string realpath = WireCell::Persist::resolve(filename);

    // Make mutable shared pointer to shart with.
    StoreDBPtr cached;
    Correction have_level = Correction::empty;

    // See if we have already loaded this file at current or lower
    // correction level
    for (auto level = correction; level > Correction::empty; --level) {
        const cache_key_t key(realpath, level);
        auto maybe = cache.find(key);

        if (maybe == cache.end()) {
            continue;
        }

        have_level = level;
        cached = maybe->second;
        break;
    }
    if (have_level == correction) {
        return Store(cached);   // nothing to do
    }
    
    auto store = std::make_shared<StoreDB>();
    if (cached) {
        store = std::make_shared<StoreDB>(*cached);
    }

    const cache_key_t ckey(realpath, correction);
    cache[ckey] = store;

    std::cerr << "WireSchema: levels: have=" << (int)have_level << " want=" << (int)correction <<"\n";

    // always load if we start empty
    if (have_level == Correction::empty) {
        load_file(realpath, *store);
        have_level = Correction::load;
    };

    // Pile on fixers to reach correction level
    plane_fixers_t fixers;
    while (have_level < correction) {
        if (have_level == Correction::load)
            fixers.push_back(plane_fixer_order);
        if (have_level == Correction::order)
            fixers.push_back(plane_fixer_direction);
        if (have_level == Correction::direction)
            fixers.push_back(plane_fixer_pitch);
        ++have_level;
    };

    fix_planes(*store, fixers);

    return Store(store);
}

// void WireCell::WireSchema::dump(const char* filename, const Store& store)
// {

// }

Store::Store()
  : m_db(nullptr)
{
}

Store::Store(StoreDBPtr db)
  : m_db(db)
{
}

Store::Store(const Store& other)
  : m_db(other.db())
{
}
Store& Store::operator=(const Store& other)
{
    m_db = other.db();
    return *this;
}

StoreDBPtr Store::db() const { return m_db; }

const std::vector<Detector>& Store::detectors() const { return m_db->detectors; }
const std::vector<Anode>& Store::anodes() const { return m_db->anodes; }
const std::vector<Face>& Store::faces() const { return m_db->faces; }
const std::vector<Plane>& Store::planes() const { return m_db->planes; }
const std::vector<Wire>& Store::wires() const { return m_db->wires; }

const Anode& Store::anode(int ident) const
{
    for (auto& a : m_db->anodes) {
        if (a.ident == ident) {
            return a;
        }
    }
    THROW(KeyError() << errmsg{String::format("Unknown anode: %d", ident)});
}

std::vector<Anode> Store::anodes(const Detector& detector) const
{
    std::vector<Anode> ret;
    for (auto ind : detector.anodes) {
        ret.push_back(m_db->anodes[ind]);
    }
    return ret;
}

std::vector<Face> Store::faces(const Anode& anode) const
{
    std::vector<Face> ret;
    for (auto ind : anode.faces) {
        ret.push_back(m_db->faces[ind]);
    }
    return ret;
}

std::vector<Plane> Store::planes(const Face& face) const
{
    std::vector<Plane> ret;
    for (auto ind : face.planes) {
        ret.push_back(m_db->planes[ind]);
    }
    return ret;
}

std::vector<Wire> Store::wires(const Plane& plane) const
{
    std::vector<Wire> ret;
    for (auto ind : plane.wires) {
        const auto& w = m_db->wires[ind];
        ret.push_back(w);
    }
    return ret;
}

BoundingBox Store::bounding_box(const Anode& anode) const
{
    BoundingBox bb;
    for (const auto& face : faces(anode)) {
        bb(bounding_box(face).bounds());
    }
    return bb;
}
BoundingBox Store::bounding_box(const Face& face) const
{
    BoundingBox bb;
    for (const auto& plane : planes(face)) {
        bb(bounding_box(plane).bounds());
    }
    return bb;
}
BoundingBox Store::bounding_box(const Plane& plane) const
{
    BoundingBox bb;
    for (const auto& wire : wires(plane)) {
        Ray ray(wire.tail, wire.head);
        bb(ray);
    }
    return bb;
}

Ray Store::wire_pitch_fast(const Plane& plane) const
{
    // Use likely longer wires from center of plane to attenuate any
    // endpoint imprecision.
    const size_t nhalf = plane.wires.size()/2;

    const Wire& w1 = m_db->wires[plane.wires[nhalf]];
    const Wire& w2 = m_db->wires[plane.wires[nhalf+1]];

    Ray r1(w1.tail, w1.head);
    Ray r2(w2.tail, w2.head);

    const Vector W = ray_unit(r1);
    const Vector P = ray_unit(ray_pitch(r1, r2));

    return Ray(W, P);
}

Vector Store::mean_pitch(const Plane& plane) const
{
    Vector ptot;
    const auto ws = wires(plane);
    const size_t nwires = ws.size();
    Ray prev;
    for (size_t wind=0; wind<nwires; ++wind) {
        const Wire& wire = ws[wind];
        Ray next(wire.tail, wire.head);
        if (wind) {             // wait until 2nd to calculate diff
            ptot = ptot + ray_vector(ray_pitch(prev, next));
        }
        prev = next;
    }
    return ptot / (nwires-1);
}

Vector Store::mean_wire(const Plane& plane) const
{
    Vector wtot;
    const auto ws = wires(plane);
    for (const auto& wire : ws) {
        Ray ray(wire.tail, wire.head);
        wtot += ray_vector(ray);
    }
    return wtot / ws.size();
}

Ray Store::wire_pitch(const Plane& plane) const
{
    auto wmean = mean_wire(plane);
    auto pmean = mean_pitch(plane);
    return Ray(wmean.norm(), pmean.norm());
}

/*
  When considering the "ray pairs" code, think of looking in
  the direction of postive X-axis with

  Y
  ^
  |
  |
 (X)----> Z
 
  WCT required conventions:

  Wire direction W: unit vector pointing from wire tail to wire head
  end points and is uniform across all wires in a plane.

  Pitch direction P: unit vector perpendicular to W, coplanar with and
  uniform across all wires in a plane.

  Pitch magnitude "pitch": perpendicular separation between any two
  neighboring wires in a plane.

  Cross product rule: X x W = P

  Pitch vector sign: if P||Z then dot(P,Y) > 0 else dot(P,Z) > 0. 

*/

ray_pair_vector_t Store::ray_pairs_active(const Face& face) const
{
    ray_pair_vector_t raypairs;

    const auto bb = bounding_box(face).bounds();
    double bbyl=bb.first.y(),  bbzl=bb.first.z();
    double bbyh=bb.second.y(), bbzh=bb.second.z();
    if (bbyl > bbyh) std::swap(bbyl, bbyh);
    if (bbzl > bbzh) std::swap(bbzl, bbzh);

    // Corners of a box in X=0 plane, pretend Y points up, Z to right
    Point ll(0.0, bbyl, bbzl);
    Point lr(0.0, bbyl, bbzh);
    Point ul(0.0, bbyh, bbzl);
    Point ur(0.0, bbyh, bbzh);

    // Horizontal bounds layer. Rays point in +Y, pitch in +Z)
    //
    // h1  h2
    // ^   ^
    // |   |
    // |   |
    // +--->  pitch = X x h1
    Ray h1(ll, ul);
    Ray h2(lr, ur);
    raypairs.push_back(ray_pair_t(h1, h2));

    // Vertical bounds layer. Rays point in -Z, pitch in +Y)
    //
    // <--^- v2
    //    |
    //    | pitch = X x v1
    //    |
    // <--+- v1
    Ray v1(lr, ll);
    Ray v2(ur, ul);
    raypairs.push_back(ray_pair_t(v1, v2));
    return raypairs;
}

ray_pair_vector_t Store::ray_pairs(const Face& face) const
{
    auto raypairs = ray_pairs_active(face);

    // Each wire plane.
    for (const auto& plane : this->planes(face)) {
        const auto phalf = 0.5*mean_pitch(plane);
        const Wire& w = m_db->wires[plane.wires.front()];
        const Ray r1(w.tail - phalf, w.head - phalf);
        const Ray r2(w.tail + phalf, w.head + phalf);
        raypairs.push_back(ray_pair_t(r1, r2));
    }

    return raypairs;
}


std::vector<int> Store::channels(const Plane& plane) const
{
    std::vector<int> ret;
    for (const auto& wire : wires(plane)) {
        ret.push_back(wire.channel);
    }
    return ret;
}


//// Validation ////

static
void validate_plane(const StoreDB& store, const Plane& plane)
{

}

void Store::validate() const
{
    const StoreDB& store = *m_db;

    for (auto& detector : store.detectors) {
        for (int ianode : detector.anodes) {
            const auto& anode = store.anodes[ianode];
            for (int iface : anode.faces) {
                const auto& face = store.faces[iface];
                std::set<int> face_plane_idents;
                for (int iplane : face.planes) {
                    const auto& plane = store.planes[iplane];
                    face_plane_idents.insert(plane.ident);
                    validate_plane(store, plane);
                }
                if (face_plane_idents.size() != face.planes.size()) {
                    THROW(ValueError() << errmsg{"plane idents not unique in face"});
                }
            }
        }
    }

}
