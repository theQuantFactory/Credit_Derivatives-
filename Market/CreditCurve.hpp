//
// Created by ricardo on 26‏/3‏/2026.
//

#pragma once
#include <string>
#include <utility>
#include <vector>

#include "instruments.hpp"
#include "YieldCurve.hpp"
#include "Core/types.hpp"


namespace  Market {

    class CreditCurve {

    public:

        CreditCurve(std::string name_id , const Core::Date ref_Date) :
        m_name_id(std::move(name_id)) ,
        m_ref_Date(ref_Date) {}

        [[nodiscard]] double survival_probability(double t) const;
        [[nodiscard]] double survival_probability(double t, double T) const;



    private :
        const std::string m_name_id;
        std::vector<Core::Point> intensity;
        Core::Date m_ref_Date;

        friend class CreditBoot;

    };

    class CreditBoot {
    public:
        CreditBoot(CDSMarketData MarketData, const YieldCurve& yieldCurve, const Core::Date ref_Date)
            : m_MarketData(std::move(MarketData)),
              m_creditCurve(m_MarketData.name, ref_Date),
              m_yieldCurve(yieldCurve),
              m_solver(1e-8, 50)
        {
            bootstrap();
        }

        [[nodiscard]] const CreditCurve& curve() const { return m_creditCurve; }

    private:
        const CDSMarketData  m_MarketData;
        CreditCurve          m_creditCurve;
        const YieldCurve&    m_yieldCurve;
        Core::NewtonRaphson  m_solver;

        void bootstrap();
        [[nodiscard]] double dQ_dLambda(double t, int i) const;
        [[nodiscard]] std::pair<double,double> spread_and_dspread(const CDS &cds, const CDS::CDSGrids &grid, int pillar_idx) const;
    };


    };





