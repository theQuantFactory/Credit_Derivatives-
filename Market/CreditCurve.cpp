//
// Created by ricardo on 26‏/3‏/2026.
//

#include "CreditCurve.hpp"

namespace Market {

    double CreditCurve::survival_probability(const double t) const {
        if (t <= 0.0) return 1.0;

        const auto it = std::lower_bound(intensity.begin(), intensity.end(), t,
            [](const Core::Point& p, const double val){ return p.time < val; });

        double cum = 0.0;
        double t_prev = 0.0;
        for (auto kt = intensity.begin(); kt != it; ++kt) {
            cum   += kt->value * (kt->time - t_prev);
            t_prev = kt->time;
        }

        if (it != intensity.end())
            cum += it->value * (t - t_prev);

        return std::exp(-cum);
    }

    double CreditCurve::survival_probability(const double t, const double T) const {
        return survival_probability(T)/survival_probability(t);
    }

    double CreditBoot::dQ_dLambda(const double t, const int i) const {

        const double tau_left  = (i == 0) ? 0.0 : m_creditCurve.intensity[i-1].time;
        const double tau_right = m_creditCurve.intensity[i].time;
        if (t <= tau_left) return 0.0;
        const double dt = std::min(t, tau_right) - tau_left;
        return -dt * m_creditCurve.survival_probability(t);
    }

    std::pair<double,double> CreditBoot::spread_and_dspread( const CDS& cds, const CDS::CDSGrids& grid, const int pillar_idx) const

        {

        const int freq_months    = cds.get_frequency();

        const size_t nd = grid.defaultTimes.size();
        const size_t np = grid.premiumTimes.size();

        std::vector<double> Q_def(nd),  dQ_def(nd),  P_def(nd);
        std::vector<double> Q_prem(np), dQ_prem(np), P_prem(np);

        for (size_t k = 0; k < nd; ++k) {
            const Core::Date& d = grid.defaultTimes[k];
            const double t = year_fraction(cds.ValuationDate, d, Core::DayCount::ACT_365);
            Q_def[k]  = m_creditCurve.survival_probability(t);
            dQ_def[k] = dQ_dLambda(t, pillar_idx);
            P_def[k]  = m_yieldCurve.discount(t);
        }

        for (size_t n = 0; n < np; ++n) {
            const Core::Date& d = grid.premiumTimes[n];
            const double t = year_fraction(cds.ValuationDate, d, Core::DayCount::ACT_365);
            Q_prem[n]  = m_creditCurve.survival_probability(t);
            dQ_prem[n] = dQ_dLambda(t, pillar_idx);
            P_prem[n]  = m_yieldCurve.discount(t);
        }

        double defLeg = 0.0, d_defLeg = 0.0;
        for (size_t k = 1; k < nd; ++k) {
            const double P_sum = P_def[k-1] + P_def[k];
            defLeg   += P_sum * (Q_def[k-1]  - Q_def[k]);
            d_defLeg += P_sum * (dQ_def[k-1] - dQ_def[k]);
        }
        defLeg   *= 0.5 * (1.0 - cds.RecoveryRate);
        d_defLeg *= 0.5 * (1.0 - cds.RecoveryRate);

        double rpv01 = 0.0, d_rpv01 = 0.0;
        for (size_t n = 1; n < np; ++n) {
            const Core::Date& d0 = grid.premiumTimes[n-1];
            const Core::Date& d1 = grid.premiumTimes[n];
            const double tau = year_fraction(d0, d1, Core::DayCount::ACT_360);
            rpv01   += tau * P_prem[n] * (Q_prem[n-1]  + Q_prem[n]);
            d_rpv01 += tau * P_prem[n] * (dQ_prem[n-1] + dQ_prem[n]);
        }
        rpv01   *= 0.5;
        d_rpv01 *= 0.5;

        const double S  = defLeg / rpv01;
        const double dS = (d_defLeg * rpv01 - defLeg * d_rpv01) / (rpv01 * rpv01);
        return {S, dS};
    }

    void CreditBoot::bootstrap() {
        m_creditCurve.intensity.resize(m_MarketData.quotes.size());

        for (size_t i = 0; i < m_MarketData.quotes.size(); ++i) {
            const Core::Point& q = m_MarketData.quotes[i];

            CDS cds;
            cds.maturity          = q.time;
            cds.RecoveryRate      = m_MarketData.recoveryRate;
            cds.EffectiveDate     = m_MarketData.effectiveDate;
            cds.ValuationDate     = m_MarketData.valuationDate;
            cds.frequency         = m_MarketData.frequency;
            cds.ContractualSpread = q.value;

            const CDS::CDSGrids grid = cds.buildCDSGrids(1);

            m_creditCurve.intensity[i] = {q.time, 0.01};

            auto f_and_df = [&](const double lam) -> std::pair<double, double> {
                m_creditCurve.intensity[i].value = lam;
                auto [S, dS] = spread_and_dspread(cds, grid, static_cast<int>(i));
                return {S - cds.ContractualSpread, dS};
            };
            m_creditCurve.intensity[i].value = m_solver.solve(f_and_df, 0.01);
        }
    }

    }










