#include "sc/progress.hpp"

#include <iostream>

namespace sc {

ProgressBar::ProgressBar(const std::string& label, size_t total)
    : label_(label), total_(total) {}

void ProgressBar::update(size_t current) {
    if (total_ == 0) return;

    int percent = static_cast<int>((current * 100) / total_);
    if (percent == last_percent_) return;
    last_percent_ = percent;

    int bar_width = 30;
    int filled = (bar_width * percent) / 100;

    std::cout << "\r" << label_ << " [";
    for (int i = 0; i < bar_width; ++i) {
        std::cout << (i < filled ? '#' : '-');
    }
    std::cout << "] " << percent << "%";
    std::cout.flush();
}

void ProgressBar::finish() {
    std::cout << "\n";
}

} // namespace sc
