#include <iostream>
#include <chrono>
#include <iomanip>

#include "Core/Dates.hpp"
#include "Market/CreditCurve.hpp"
#include "Market/YieldCurve.hpp"
#include "Market/instruments.hpp"
#include "Pricers/pricers.hpp"

double spread(const double maturity,
              const Market::CreditCurve& cc,
              const Market::YieldCurve&  yc,
              const Market::CDSMarketData& mdata)
{
    Market::CDS cds;
    cds.maturity          = maturity;
    cds.RecoveryRate      = mdata.recoveryRate;
    cds.frequency         = mdata.frequency;
    cds.EffectiveDate = mdata.effectiveDate;
    cds.ValuationDate = mdata.valuationDate;
    cds.ContractualSpread = 0.0;

    const Market::CDS::CDSGrids grid = cds.buildCDSGrids(1);

    double defLeg = 0.0;
    for (size_t k = 1; k < grid.defaultTimes.size(); ++k) {
        const double t0 = Core::year_fraction(mdata.valuationDate, grid.defaultTimes[k-1], Core::DayCount::ACT_365);
        const double t1 = Core::year_fraction(mdata.valuationDate, grid.defaultTimes[k],   Core::DayCount::ACT_365);
        defLeg += 0.5 * (yc.discount(t0) + yc.discount(t1))
                      * (cc.survival_probability(t0) - cc.survival_probability(t1));
    }
    defLeg *= (1.0 - mdata.recoveryRate);

    double rpv01 = 0.0;
    for (size_t n = 1; n < grid.premiumTimes.size(); ++n) {
        const double tau = Core::year_fraction(grid.premiumTimes[n-1], grid.premiumTimes[n], Core::DayCount::ACT_360);
        const double t0  = Core::year_fraction(mdata.valuationDate, grid.premiumTimes[n-1], Core::DayCount::ACT_365);
        const double t1  = Core::year_fraction(mdata.valuationDate, grid.premiumTimes[n],   Core::DayCount::ACT_365);
        rpv01 += tau * 0.5 * yc.discount(t1)
               * (cc.survival_probability(t0) + cc.survival_probability(t1));
    }

    return defLeg / rpv01;
}


int main() {

    auto start = std::chrono::high_resolution_clock::now();

    const Core::Date today(2026, 3, 26);
    Market::YieldCurveBoot bs(today);

    bs.add_deposits({
        {1.0/365,  0.0390},
        {7.0/365, 0.0392},
        {1.0/12,  0.0395},
        {3.0/12,  0.0400},
    });

    bs.add_futures({
        {3.0/12,  6.0/12,  95.90, 0.01},
        {6.0/12,  9.0/12,  96.00, 0.01},
        {9.0/12,  12.0/12, 96.15, 0.01},
        {12.0/12, 15.0/12, 96.30, 0.01},
    });

    bs.add_swaps({
              { 2.0,  0.0470, 0.25, 0.25},
              { 3.0,  0.0360, 0.25, 0.25},
              { 5.0,  0.0345, 0.25, 0.25},
              { 10.0, 0.0338, 0.25, 0.25},
          });

    const auto& yc = bs.curve();

    std::cout << "=== Yield Curve ===\n";
    std::cout << std::left << std::setw(10) << "t"
              << std::right << std::setw(15) << "DF" << "\n";
    std::cout << "-------------------------------\n";
    for (const double t : {0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0})
        std::cout << std::left << std::setw(10) << t
                  << std::right << std::setw(15) << std::fixed << std::setprecision(6) << yc.discount(t)
                  << "\n";

    Market::CDSMarketData mdata;
    mdata.name          = "ACME_5Y";
    mdata.effectiveDate = today;
    mdata.valuationDate = today;
    mdata.recoveryRate  = 0.40;
    mdata.frequency     = Core::Frequency::QUARTERLY;
    mdata.quotes        = {
        { 1.0, 0.0050 },
        { 2.0, 0.0080 },
        { 3.0, 0.0100 },
        { 5.0, 0.0130 },
        { 7.0, 0.0150 },
        { 10.0, 0.0170 },
    };

    Market::CreditBoot cc_boot(mdata, yc, today);
    const auto& cc = cc_boot.curve();

    std::cout << "\n=== Credit Curve ===\n";
    std::cout << std::left << std::setw(10) << "t"
              << std::right << std::setw(15) << "Q(t)"
              << std::setw(15) << "PD (%)" << "\n";
    std::cout << "----------------------------------------\n";
    for (const double t : { 1.0, 2.0, 3.0, 5.0, 7.0, 10.0 }) {
        double q = cc.survival_probability(t);

        std::cout << std::left << std::setw(10) << t
                  << std::right << std::setw(15) << std::fixed << std::setprecision(6) << q
                  << std::setw(15) << std::fixed << std::setprecision(2) << (1.0 - q) * 100.0
                  << "\n";
    }


    Market::CDS cds;
    cds.Name              = "ACME_5Y";
    cds.maturity          = 5.0;
    cds.Nominal           = 10'000'000.0;  // 10M
    cds.EffectiveDate = today;
    cds.ValuationDate = today;
    cds.RecoveryRate      = 0.40;
    cds.frequency         = Core::Frequency::QUARTERLY;
    cds.ContractualSpread = 0.0100;

    Pricer::CDSPricer pricer(cds, yc, cc);

    std::cout << "\n=== CDS Pricer ===\n";
    std::cout << std::left << std::setw(20) << "Par spread (bp)"
              << std::right << std::setw(15) << pricer.par_spread() * 10000 << "\n";
    std::cout << std::left << std::setw(20) << "RPV01"
              << std::right << std::setw(15) << pricer.rpv01() << "\n";
    std::cout << std::left << std::setw(20) << "NPV"
              << std::right << std::setw(15) << pricer.npv() << "\n";
    std::cout << std::left << std::setw(20) << "Upfront"
              << std::right << std::setw(15) << pricer.upfront() << "\n";


    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Execution time : " << duration.count() << " ms\n";

    return 0;
}