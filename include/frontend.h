#pragma once
#include <string>
#include <vector>
#include <memory>

struct FrontendContext {
    std::string path;
    std::string src;
    bool verbose;
};

struct Bytecode {
    std::vector<uint8_t> data;
};

class IFrontend {
public:
    virtual ~IFrontend() = default;
    virtual std::string name() const = 0;
    virtual bool accepts(const std::string& path, const std::string& src) const = 0;
    virtual bool compile(const FrontendContext& ctx, Bytecode& out, std::string& err) const = 0;
};

class FrontendRegistry {
public:
    static FrontendRegistry& instance();
    void register_frontend(std::unique_ptr<IFrontend> fe);
    std::vector<IFrontend*> all() const;
    IFrontend* match(const std::string& path, const std::string& src) const;
    IFrontend* by_name(const std::string& name) const;

private:
    FrontendRegistry() = default;
    std::vector<std::unique_ptr<IFrontend>> frontends_;
};