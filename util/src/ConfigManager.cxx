#include "WireCellUtil/ConfigManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"

#include <unordered_map>

using namespace std;
using namespace WireCell;


ConfigManager::ConfigManager()
  : m_top(Json::arrayValue)
{
}
ConfigManager::~ConfigManager() {}

void ConfigManager::extend(Configuration more)
{
    // O(N+M) instead of O(N*M): build an index map of existing entries once,
    // then probe per new entry.  Bulk loads of large configs (thousands of
    // entries) used to scale quadratically through add()->index() linear scan.
    std::unordered_map<std::string, int> by_tn;
    by_tn.reserve(m_top.size() + more.size());
    for (int i = 0; i < static_cast<int>(m_top.size()); ++i) {
        const auto& c = m_top[i];
        by_tn[get<string>(c, "type") + "\t" + get<string>(c, "name")] = i;
    }

    for (const auto& one : more) {
        const std::string key = get<string>(one, "type") + "\t" + get<string>(one, "name");
        auto it = by_tn.find(key);
        if (it == by_tn.end()) {
            const int ind = m_top.size();
            m_top[ind] = one;
            by_tn[key] = ind;
        }
        else {
            spdlog::warn("ConfigManager::extend() overwriting type=\"{}\" name=\"{}\"",
                         get<string>(one, "type"), get<string>(one, "name"));
            m_top[it->second] = one;
        }
    }
}

int ConfigManager::index(const std::string& type, const std::string& name) const
{
    int ind = -1;
    for (const auto& c : m_top) {
        ++ind;
        if (get<string>(c, "type") != type) {
            continue;
        }
        if (get<string>(c, "name") != name) {
            continue;
        }
        return ind;
    }
    return -1;
}

int ConfigManager::add(Configuration& cfg)
{
    const std::string type = get<string>(cfg, "type");
    const std::string name = get<string>(cfg, "name");

    int ind = this->index(type, name);
    if (ind < 0) {
        ind = m_top.size();
    }
    else {
        spdlog::warn("ConfigManager:add() overwriting existing type=\"{}\" name=\"{}\"", type, name);
    }
    m_top[ind] = cfg;
    return ind;
}

int ConfigManager::add(Configuration& payload, const std::string& type, const std::string& name)
{
    Configuration cfg;
    cfg["data"] = payload;
    cfg["type"] = type;
    cfg["name"] = name;
    return add(cfg);
}

Configuration ConfigManager::at(int ind) const
{
    if (ind < 0 || ind >= size()) {
        return Configuration();
    }
    return m_top[ind];
}

std::vector<ConfigManager::ClassInstance> ConfigManager::configurables() const
{
    std::vector<ConfigManager::ClassInstance> ret;
    for (auto c : m_top) {
        ret.push_back(make_pair(get<string>(c, "type"), get<string>(c, "name")));
    }
    return ret;
}

Configuration ConfigManager::pop(int ind)
{
    if (ind < 0 || ind >= size()) {
        return Configuration();
    }
    Configuration ret;
    Configuration reduced(Json::arrayValue);
    int siz = size();
    for (int i = 0; i < siz; ++i) {
        if (i == ind) {
            ret = m_top[i];
        }
        else {
            reduced.append(m_top[i]);
        }
    }
    m_top = reduced;
    return ret;
}
