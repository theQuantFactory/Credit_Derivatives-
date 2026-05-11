//
// Created by ricardo on 27‏/3‏/2026.
//

#include "pricers.hpp"

namespace Pricer {

    double CDSPricer::rpv01_raw(const Core::Date valuation_date) const {

        const auto it = std::ranges::upper_bound(m_grid.premiumTimes
                                                 ,valuation_date,
                                                 [](const Core::Date& a, const Core::Date& b) {
                                                     return a.getJulianDays() < b.getJulianDays();
                                                 });

        if (it != m_grid.premiumTimes.end() && it != m_grid.premiumTimes.begin()) {
            const Core::Date& next_date = *it;
            const Core::Date& prev_date = *std::prev(it);

            const double val_prev = yf(prev_date,valuation_date,Core::DayCount::ACT_365);
            const double next_val = yf(valuation_date , next_date , Core::DayCount::ACT_365);

           const double prv01_1 = val_prev * m_y_curve.discount(next_val) * (1 - m_credit_curve.survival_probability(next_val));
           const double prv01_2 = 0.5 * next_val * m_y_curve.discount(next_val) * (1 - m_credit_curve.survival_probability(next_val));
           const double prv01_3 = yf(prev_date,next_date,Core::DayCount::ACT_365) * m_y_curve.discount(next_val) *
               m_credit_curve.survival_probability(next_val) ;

           double prv01_4 = 0.0;
            for (auto kt = it; kt != m_grid.premiumTimes.end(); ++kt) {

                const Core::Date& Date_0 = *std::prev(kt);
                const Core::Date& Date_1 = *kt;

                const double d1 = yf(valuation_date , Date_1 , Core::DayCount::ACT_365);
                const double d0 = yf(valuation_date , Date_0 , Core::DayCount::ACT_365);

                prv01_4 +=  (d1-d0) * m_y_curve.discount(d1) * ( m_credit_curve.survival_probability(d0)+
                    m_credit_curve.survival_probability(d1) );

            }

            return prv01_1 + prv01_2 + prv01_3 + 0.5*prv01_4;

        }

        else {
           throw std::runtime_error("Valuation date greater than maturity or lower than effective date");
        }
    }

    double CDSPricer::default_leg(const Core::Date valuation_date) const {

        const auto it = std::ranges::upper_bound(m_grid.defaultTimes
                                                 ,valuation_date,
                                                 [](const Core::Date& a, const Core::Date& b) {
                                                     return a.getJulianDays() < b.getJulianDays();
                                                 });

        if (it != m_grid.defaultTimes.end() && it != m_grid.defaultTimes.begin()  ) {

            double defLeg = 0.0;

            const Core::Date& next_date = *it;
            const Core::Date& prev_date = *std::prev(it);

            constexpr double t0 = 0 ;
            const double t1 = yf(valuation_date,next_date,Core::DayCount::ACT_365);

            defLeg = (m_credit_curve.survival_probability(t0) - m_credit_curve.survival_probability(t1)) * (m_y_curve.discount(t0) + m_y_curve.discount(t1)) ;

            for (auto kt = std::next(it); kt != m_grid.defaultTimes.end(); ++kt) {

                const Core::Date& Date_0 = *std::prev(kt);
                const Core::Date& Date_1 = *kt;

                const double d1 = yf(valuation_date , Date_1 , Core::DayCount::ACT_365);
                const double d0 = yf(valuation_date , Date_0 , Core::DayCount::ACT_365);

                defLeg += (m_credit_curve.survival_probability(d0) - m_credit_curve.survival_probability(d1)) * (m_y_curve.discount(d0) + m_y_curve.discount(d1));

            }

            return defLeg * (1.0 - m_cds.RecoveryRate);

            }

        else {
            throw std::runtime_error("Valuation date greater than maturity or lower than effective date");
        }

    }

    double CDSPricer::par_spread() const {

        if ( m_cds.EffectiveDate == m_cds.ValuationDate) {
            return default_leg(m_cds.EffectiveDate) / rpv01_raw(m_cds.EffectiveDate);
        }
        else {
            throw std::runtime_error("Par spread only available for CDS that effective date match valuation date");
        }
    }

    double CDSPricer::upfront() const {

        const double up = (m_cds.ContractualSpread - par_spread()) * rpv01_raw(m_cds.EffectiveDate) * m_cds.Nominal ;
        return up ;
    }

    double CDSPricer::upfront(const double actual_spread) const {

        const double up = (m_cds.ContractualSpread - actual_spread) * rpv01_raw(m_cds.ValuationDate) * m_cds.Nominal ;
        return up ;
    }

    double CDSPricer::rpv01() const {

       const double rpv = rpv01_raw(m_cds.ValuationDate)*m_cds.Nominal;
        return rpv ;
    }

    double CDSPricer::npv() const {
        const double dl   = default_leg(m_cds.EffectiveDate);
        const double rpv  = rpv01_raw(m_cds.EffectiveDate);
        const double s_par = dl / rpv;  // par_spread inline
        return (dl - s_par * rpv) * m_cds.Nominal;
    }

    double CDSPricer::npv(const double actual_spread) const {
        const double npv = (default_leg(m_cds.ValuationDate) - actual_spread * rpv01_raw(m_cds.ValuationDate)) * m_cds.Nominal;
        return npv ;
    }


}
