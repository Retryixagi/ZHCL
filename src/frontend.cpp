#include "../include/frontend.h"
#include <unordered_set>

FrontendRegistry& FrontendRegistry::instance() {
    static FrontendRegistry instance;
    return instance;
}

void FrontendRegistry::register_frontend(std::unique_ptr<IFrontend> fe) {
    static std::unordered_set<std::string> registered_names;
    std::string name = fe->name();
    if (name.empty() || registered_names.count(name)) {
        return; // 去重：同名前端只保留第一个
    }
    registered_names.insert(name);
    frontends_.push_back(std::move(fe));
}

std::vector<IFrontend*> FrontendRegistry::all() const {
    std::vector<IFrontend*> result;
    for (const auto& fe : frontends_) {
        result.push_back(fe.get());
    }
    return result;
}

IFrontend* FrontendRegistry::match(const std::string& path, const std::string& src) const {
    for (const auto& fe : frontends_) {
        if (fe->accepts(path, src)) {
            return fe.get();
        }
    }
    return nullptr;
}

IFrontend* FrontendRegistry::by_name(const std::string& name) const {
    for (const auto& fe : frontends_) {
        if (fe->name() == name) return fe.get();
    }
    return nullptr;
}