//
// Created by ricardo on 25‏/3‏/2026.
//

#include "interpolator.hpp"

namespace Core {

    LogLinearInterpolator::LogLinearInterpolator(const std::vector<double>& x,
                              const std::vector<double>& y)
            : m_x(x), m_logy(y.size())
    {
        std::ranges::transform(y, m_logy.begin(),
                               [](const double v) { return std::log(v); });
    }

    double LogLinearInterpolator::operator()(const double t) const{

        if (t <= 0.0) return 1.0;
        if (t >= m_x.back()) return std::exp(m_logy.back());
        if (t < m_x.front()) return std::exp(m_logy.front() * (t / m_x.front()));

        const auto it = std::ranges::lower_bound(m_x, t);
        const size_t i = static_cast<size_t>(it - m_x.begin()); // O(1) !

        if (it != m_x.end() && *it == t) return std::exp(m_logy[i]);  // exact match

        const double t0    = m_x[i-1],  t1    = m_x[i];
        const double alpha = (t - t0) / (t1 - t0);
        return std::exp((1.0 - alpha) * m_logy[i-1] + alpha * m_logy[i]);

};

    LinearInterpolator::LinearInterpolator(const std::vector<double>& x,
                            const std::vector<double>& y)
             : m_x(x), m_y(y) {};

    double  LinearInterpolator::operator()(const double t) const {

        if (t<=0) return 1.0 ;

        for (std::size_t i = 0; i < m_x.size(); ++i) {
            if (std::abs(t - m_x[i]) < 1e-10)
                return m_y[i];
        }

        if (t < m_x.front()) {
            return m_y.front()*(t/m_x.front()) ;
        }

        if (t >= m_x.back())  return m_y.back();

        const auto it = std::ranges::lower_bound(m_x, t);
        const size_t i = std::distance(m_x.begin(), it);

        const double alpha = (t - m_x[i-1]) / (m_x[i] - m_x[i-1]);
        return (1.0 - alpha) * m_y[i-1] + alpha * m_y[i];
    }




}
