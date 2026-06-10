#pragma once

#include <cstddef>
#include <string>

namespace sc {

class ProgressBar {
public:
    ProgressBar(const std::string& label, size_t total);
    void update(size_t current);
    void finish();

private:
    std::string label_;
    size_t total_;
    int last_percent_ = -1;
};

} // namespace sc
