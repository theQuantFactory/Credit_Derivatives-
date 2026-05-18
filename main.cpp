#include <iostream>
#include "interface.h"

using namespace RiskReport;



int main() {
    try {
        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n"
                  << "║  RiskReport — CDS & CDO Tranches Pricing & Risk             ║\n"
                  << "║  Version 2.0 — Production Ready                             ║\n"
                  << "╚════════════════════════════════════════════════════════════╝\n";

        // ─────────────────────────────────────────────────────────────────────────────
        //  1. Construire l'input du RiskReport
        // ─────────────────────────────────────────────────────────────────────────────

        RiskReport::Input input;
        input.ref_date = Core::Date(2026, 5, 15);

        // Yield curve
        input.yield_deposits = {
            {0.25, 0.0375}, {0.5, 0.0375}, {1.0, 0.0375}
        };
        input.yield_swaps = {
            {2.0, 0.0375, 1.0, 0.25},
            {3.0, 0.0370, 1.0, 0.25},
            {5.0, 0.0360, 1.0, 0.25},
            {7.0, 0.0355, 1.0, 0.25},
            {10.0, 0.0350, 1.0, 0.25}
        };

        // CDS market data
        Market::CDSMarketData cds_md;
        cds_md.name = "iTraxx.Main.S43";
        cds_md.effectiveDate = input.ref_date;
        cds_md.valuationDate = input.ref_date;
        cds_md.recoveryRate = 0.40;
        cds_md.frequency = Core::Frequency::QUARTERLY;
        cds_md.quotes = {
            {1.0, 0.0045}, {3.0, 0.0065}, {5.0, 0.0080},
            {7.0, 0.0092}, {10.0, 0.0105}
        };
        input.cds_market = cds_md;

        // CDS instrument (single name example)
        Market::CDS cds_instr;
        cds_instr.Name = "Example CDS";
        cds_instr.maturity = 5.0;
        cds_instr.Nominal = 10.0e6;
        cds_instr.RecoveryRate = 0.40;
        cds_instr.ContractualSpread = 0.008;
        cds_instr.EffectiveDate = input.ref_date;
        cds_instr.ValuationDate = input.ref_date;
        cds_instr.frequency = Core::Frequency::QUARTERLY;
        input.cds_instrument = cds_instr;

        input.single_name_curves = std::vector(125, cds_md);

        // Tranches market data
        Market::Tranches_MarketData tranche_md;
        tranche_md.index_name = "iTraxx.Main.S43";
        tranche_md.n_credits = 125;
        tranche_md.recovery_rate = 0.40;

        struct TrancheSpec {
            double K1, K2, spread_bp;
        };
        static constexpr TrancheSpec specs[] = {
            {0.00, 0.03, 1800.0},
            {0.03, 0.06, 280.0},
            {0.06, 0.09, 95.0},
            {0.09, 0.12, 40.0},
            {0.12, 0.22, 14.0},
        };

        for (const auto& spec : specs) {
            Market::Index_tranche tr;
            tr.K1 = spec.K1;
            tr.K2 = spec.K2;
            tr.contractual_spread = spec.spread_bp * 1e-4;
            tr.fair_spread = spec.spread_bp * 1e-4;
            tr.quoted_upfront = false;
            tr.upfront = 0.0;
            tr.nominal = (spec.K2 - spec.K1) * 1e9;
            tr.maturity = 5.0;
            tr.effective_date = input.ref_date;
            tr.valuation_date = input.ref_date;
            tr.frequency = Core::Frequency::QUARTERLY;
            tr.day_count = Core::DayCount::ACT_360;
            tranche_md.quoted_tranches.push_back(tr);
        }
        input.tranche_market = tranche_md;
        input.IndexMarketData = cds_md;
        input.pricer_type = Method::Gaussian;

        // ─────────────────────────────────────────────────────────────────────────────
        //  2. Créer et lancer le moteur RiskReport
        // ─────────────────────────────────────────────────────────────────────────────

        ReportConfig cfg;
        cfg.grid_n = 100;
        cfg.verbose = true;

        Engine engine(cfg);
        Report report = engine.run(input);

        // ─────────────────────────────────────────────────────────────────────────────
        //  3. Afficher et exporter le rapport
        // ─────────────────────────────────────────────────────────────────────────────

        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n"
                  << "║  RAPPORT DÉTAILLÉ                                          ║\n"
                  << "╚════════════════════════════════════════════════════════════╝\n";

        report.print();

        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n"
                  << "║  EXPORT DES DONNÉES                                        ║\n"
                  << "╚════════════════════════════════════════════════════════════╝\n";

        try {
            report.to_json("riskreport.json");
            std::cout << "\n  ✓ Rapport JSON complet exporté vers: reports/riskreport.json\n";
        } catch (const std::exception& e) {
            std::cout << "\n  ⚠ Export JSON échoué: " << e.what() << "\n";
        }

        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n"
                  << "║  SUCCÈS — Rapport généré avec succès                       ║\n"
                  << "╚════════════════════════════════════════════════════════════╝\n\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n✗ Erreur: " << e.what() << "\n\n";
        return 1;
    }
}