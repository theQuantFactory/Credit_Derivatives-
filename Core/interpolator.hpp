//
// Created by ricardo on 25‏/3‏/2026.
//

#pragma once
#include <algorithm>
#include <vector>
#include <cmath>

namespace Core {

    class LogLinearInterpolator {
    public:

        LogLinearInterpolator() = delete;
        LogLinearInterpolator(const std::vector<double>& x, const std::vector<double>& y);
        double operator()(double t) const;

    private:
        const std::vector<double> m_x;;
        std::vector<double> m_logy;
    };

    class LinearInterpolator {
    public:

        LinearInterpolator() = delete;
        LinearInterpolator(const std::vector<double>& x,
                           const std::vector<double>& y);

        double operator()(double t) const;

    private:
        const std::vector<double>& m_x;
        const std::vector<double>& m_y;
    };

}
