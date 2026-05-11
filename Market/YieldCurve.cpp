//
// Created by ricardo on 26‏/3‏/2026.
//

#include "YieldCurve.hpp"

namespace Market {


    void YieldCurve::add_pillar(const double t, const double df) {

        if (df < 0.0)
            throw std::invalid_argument("Invalid value for the discount factor : " + std::to_string(df));

        if (!m_pillars.empty() && t <= m_pillars.back().time) {
            throw std::invalid_argument("Bad adding, increasing order not followed : t=" + std::to_string(t) +
                                        " <= Last t=" + std::to_string(m_pillars.back().time));
        }
        m_pillars.push_back({t, df});
    }

    void YieldCurve::pop_pillar() {
        if (m_pillars.size() <= 1)
            throw std::runtime_error("Pillar 0 is not removable");
        m_pillars.pop_back();
    }

    double YieldCurve::discount(double t) const {

        if (m_pillars.empty())
            throw std::runtime_error("Void curve");

        const auto it = std::lower_bound(m_pillars.begin(), m_pillars.end(), t,
            [](const Core::Point& p, const double value) { return p.time < value; });

        if (it != m_pillars.end() && it->time == t) {
            return it->value;
        }

        if (it == m_pillars.begin()) {
            const double t0  = m_pillars.front().time;
            const double df0 = m_pillars.front().value;
            const double z0  = -std::log(df0) / t0;
            return std::exp(-z0 * t);
        }

        if (it == m_pillars.end()) {
            const double t_last  = m_pillars.back().time;
            const double df_last = m_pillars.back().value;
            const double z_last  = -std::log(df_last) / t_last;
            return std::exp(-z_last * t);
        }

        const auto it0 = std::prev(it);
        const double t0  = it0->time;
        const double t1  = it->time;
        const double df0 = it0->value;
        const double df1 = it->value;

        const double alpha = (t - t0) / (t1 - t0);
        return std::exp((1.0 - alpha) * std::log(df0) + alpha * std::log(df1));
    }

    double YieldCurve::forward_rate(const double t1, const double t2) const {
        if (t2 <= t1)
            throw std::invalid_argument("t2 doit être > t1");
        const double df1 = discount(t1);
        const double df2 = discount(t2);
        return std::log(df1 / df2) / (t2 - t1);
    }

}
