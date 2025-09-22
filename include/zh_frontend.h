#pragma once
#include <string>
#include <vector>
#include <cstdint>

class ZhFrontend {
public:
    std::vector<uint8_t> translate_to_bc(const std::string& src);
};