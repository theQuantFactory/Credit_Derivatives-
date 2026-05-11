//
// Created by ricardo on 27‏/3‏/2026.
//

#pragma once

#include "Market/instruments.hpp"
#include "Market/YieldCurve.hpp"
#include "Market/CreditCurve.hpp"
#include "Core/types.hpp"

namespace Pricer {

    class CDSPricer {

    public:

        CDSPricer(const Market::CDS&          cds,
                  const Market::YieldCurve&   y_curve,
                  const Market::CreditCurve&  credit_curve)
            : m_cds(cds),
              m_y_curve(y_curve),
              m_credit_curve(credit_curve) {
              m_grid = m_cds.buildCDSGrids(1);
        }

        [[nodiscard]] double par_spread()  const;

        [[nodiscard]] double rpv01() const;

        [[nodiscard]] double npv()         const;
        [[nodiscard]] double npv(double actual_spread) const;

        [[nodiscard]] double upfront()     const;
        [[nodiscard]] double upfront(double actual_spread) const ;

    private:

        const Market::CDS&         m_cds;
        const Market::YieldCurve&  m_y_curve;
        const Market::CreditCurve& m_credit_curve;
        Market::CDS::CDSGrids m_grid;

        [[nodiscard]] double default_leg(Core::Date valuation_date) const;

        [[nodiscard]] double rpv01_raw(Core::Date valuation_date)   const;

        static double yf(const Core::Date& d1, const Core::Date& d2,
                                const Core::DayCount dc) {
            return Core::year_fraction(d1, d2, dc);
        }
    };

}
