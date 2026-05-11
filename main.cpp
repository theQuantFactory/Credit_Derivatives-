
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>

#include "Core/Dates.h"
#include "Core/types.h"
#include "Market/Instruments/CDS/instruments.h"
#include "Market/Curves/YieldCurve/YieldCurve.h"
#include "Market/Curves/CDS/CreditCurve.h"
#include "Market/Instruments/Tranches/tranches_instruments.h"
#include "Market/Curves/Tranches/BaseCorrelationCurve.h"
#include "Pricers/Tranches/Methods/lhp_pricer.h"
#include "Pricers/Tranches/Methods/full_recursion_pricing.h"
#include "Market/Calibration/base_correlation_curve.h"
#include "Risk/Tranches/risk_engine.h"
#include "Risk/Tranches/bumper.h"

// =============================================================================
//  Constantes du portfolio
// =============================================================================
static constexpr int    N_CREDITS    = 125;
static constexpr double RR           = 0.40;
static constexpr double INDEX_NOTL   = 1e9;   // 1 Milliard EUR notionnel index

// =============================================================================
//  Affichage
// =============================================================================
static void banner(const std::string& s, char c = '=') {
    std::cout << "\n" << std::string(76, c) << "\n  " << s
              << "\n" << std::string(76, c) << "\n";
}
static void sub(const std::string& s) {
    std::cout << "\n  -- " << s << " --\n";
}
static std::string bar(double v, double scale, int w = 18) {
    int n = std::min(static_cast<int>(std::abs(v)/scale*w), w);
    std::string b = v < 0 ? "[-" : "[+";
    b += std::string(n,'#') + std::string(w-n,' ') + "]";
    return b;
}

// =============================================================================
//  Construction marché
// =============================================================================
static Market::YieldCurve build_yc(const Core::Date& ref) {
    Market::YieldCurveBoot b(ref);
    b.add_deposits({{0.25,0.0375},{0.5,0.0375},{1.0,0.0375}});
    b.add_swaps({{2.,0.0375,1.,0.25},{3.,0.0370,1.,0.25},
                 {5.,0.0360,1.,0.25},{7.,0.0355,1.,0.25},{10.,0.0350,1.,0.25}});
    return b.curve();
}

static Market::CDSMarketData build_cds_md(const Core::Date& ref) {
    Market::CDSMarketData md;
    md.name="iTraxx.Main.S43"; md.effectiveDate=ref; md.valuationDate=ref;
    md.recoveryRate=RR; md.frequency=Core::Frequency::QUARTERLY;
    // Spreads CDS index marché (mai 2026, approx)
    md.quotes={{1.,0.0045},{3.,0.0065},{5.,0.0080},{7.,0.0092},{10.,0.0105}};
    return md;
}

// Cotations marché des tranches iTraxx Main S43 5Y (bp)
// fair_spread = cotation marché = contractual_spread ici (at-market)
static Market::Tranches_MarketData build_tranches_md(const Core::Date& ref) {
    Market::Tranches_MarketData tmd;
    tmd.index_name   = "iTraxx.Main.S43";
    tmd.n_credits    = N_CREDITS;
    tmd.recovery_rate= RR;

    struct Spec { double K1,K2; double spd_bp; bool upfront; double uf; };
    // Spreads iTraxx Main S43 mai 2026
    // Convention : toutes les tranches en running spread pour le bootstrap BC
    // (l'equity est aussi traitée en running ici ; en pratique elle est cotée
    //  en upfront + 500bp running mais le bootstrapper attend un NPV=0 en running)
    static const Spec specs[] = {
        {0.00, 0.03, 1800.,  false, 0. },  // equity — spread running équivalent
        {0.03, 0.06,  280.,  false, 0. },
        {0.06, 0.09,   95.,  false, 0. },
        {0.09, 0.12,   40.,  false, 0. },
        {0.12, 0.22,   14.,  false, 0. },
        {0.22, 1.00,    3.,  false, 0. },
    };

    // Convention : on ne bootstrappe PAS le super-senior [22-100%]
    // K2=100% → base tranche [0-100%] = portfolio entier → rho sans effet sur NPV global
    // On s'arrête à K2=22% ([12-22%] est le dernier point bootstrappé)
    for (std::size_t si = 0; si < std::size(specs) - 1; ++si) {
        const auto& s = specs[si];
        Market::Index_tranche tr;
        tr.K1 = s.K1; tr.K2 = s.K2;
        tr.contractual_spread = s.spd_bp * 1e-4;
        tr.fair_spread        = s.spd_bp * 1e-4;
        tr.quoted_upfront     = s.upfront;
        tr.upfront            = s.uf;
        tr.nominal            = (s.K2 - s.K1) * INDEX_NOTL;
        tr.maturity           = 5.;
        tr.effective_date     = ref;
        tr.valuation_date     = ref;
        tr.frequency          = Core::Frequency::QUARTERLY;
        tr.day_count          = Core::DayCount::ACT_360;
        tmd.quoted_tranches.push_back(tr);
    }
    return tmd;
}

// =============================================================================
//  Affichage détaillé d'une tranche
// =============================================================================
struct TrancheInfo {
    const char* name;
    double K1, K2;
    double spd_bp;
    bool   upfront;
    double uf_pct;
};
static const TrancheInfo TINFO[] = {
    {"[0-3%]   ", 0.00, 0.03, 1800., false,  0.0},
    {"[3-6%]   ", 0.03, 0.06,  280., false,  0.0},
    {"[6-9%]   ", 0.06, 0.09,   95., false,  0.0},
    {"[9-12%]  ", 0.09, 0.12,   40., false,  0.0},
    {"[12-22%] ", 0.12, 0.22,   14., false,  0.0},
    {"[22-100%]", 0.22, 1.00,    3., false,  0.0},
};
static constexpr int N_TR = 6;

static void print_greeks(const Risk::TranchesGreeksResult& g, int i,
                         const std::vector<double>& pillars)
{
    const auto& ti = TINFO[i];
    const double notl_m = (ti.K2-ti.K1)*INDEX_NOTL/1e6;

    std::cout << "\n  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║  Tranche " << ti.name
              << "   Notionnel = " << std::fixed << std::setprecision(0)
              << notl_m << "M EUR"
              << (ti.upfront ? "  [upfront]" : "           ")
              << "          ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════╝\n\n";

    // --- Spread & MtM ---
    const double par_bp = g.par_spread * 1e4;
    std::cout << std::fixed;
    if (ti.upfront) {
        std::cout << "  Convention       : upfront + " << ti.spd_bp << "bp running\n"
                  << "  Upfront marché   : " << std::setprecision(1) << ti.uf_pct << "%\n";
    } else {
        std::cout << "  Par spread       : " << std::setprecision(2) << par_bp << " bp"
                  << "   (coté: " << ti.spd_bp << " bp)\n";
        const double pnl = (par_bp - ti.spd_bp) * (-g.cs01) * 1e-4;
        // PnL approx = ΔS * CS01 (CS01 est négatif pour long protection)
        std::cout << "  MtM (long prot.) : "
                  << std::setprecision(0) << (pnl>=0?"+":"") << pnl << " EUR"
                  << "  [≈ ΔS × CS01]\n";
    }

    // --- Bumps ---
    // Les Greeks du RiskEngine sont des dérivées (divisées par h).
    // On remultiplie par le bump pour obtenir la VARIATION pour exactement 1bp / 1pt.
    constexpr double H_CS  = 1e-4;   // 1bp spread
    constexpr double H_RT  = 1e-4;   // 1bp taux
    constexpr double H_RHO = 0.01;   // 1pt corrélation

    const double dcs01  = g.cs01  * H_CS;    // ΔNPV pour +1bp spread (EUR)
    const double ddv01  = g.dv01  * H_RT;    // ΔNPV pour +1bp taux   (EUR)
    const double drho01 = g.rho01 * H_RHO;   // ΔNPV pour +1pt corr   (EUR)
    // Gamma : d²NPV/dS² × (1bp)² = variation de CS01 pour +1bp supplémentaire
    const double dgamma = g.gamma * H_CS * H_CS;

    // --- CS01 ---
    std::cout << "\n  CS01  (ΔNPV +1bp spread)  : "
              << std::setprecision(0) << dcs01 << " EUR\n"
              << "  " << bar(dcs01, 3e1) << "\n"
              << "  → Si spreads +1bp : NPV " << (dcs01>=0?"+":"") << dcs01/1e3 << "k EUR\n"
              << "    Position : protection " << (dcs01<0?"LONG (acheteur)":"SHORT (vendeur)") << "\n";

    // --- DV01 ---
    std::cout << "\n  DV01  (ΔNPV +1bp taux)    : "
              << std::setprecision(0) << ddv01 << " EUR\n"
              << "  " << bar(ddv01, 3e0) << "\n"
              << "  → Si taux +1bp : NPV " << (ddv01>=0?"+":"") << ddv01/1e3 << "k EUR\n"
              << "    " << (ddv01>0
                  ? "Positif : la protection vaut plus quand les taux montent"
                  : "Négatif : duration du premium leg > protection leg") << "\n";

    // --- Rho01 ---
    std::cout << "\n  Rho01 (ΔNPV +1pt corr)   : "
              << std::setprecision(0) << drho01 << " EUR\n"
              << "  " << bar(drho01, 5e2) << "\n";
    if (drho01 > 0)
        std::cout << "  → LONG corrélation : rho↑ comprime le spread equity\n"
                  << "    (défauts plus groupés = moins de petites pertes isolées)\n";
    else
        std::cout << "  → COURT corrélation : rho↑ élargit le spread senior/super-senior\n"
                  << "    (défauts groupés = plus probable d'atteindre les tranches hautes)\n";

    // --- Gamma ---
    std::cout << "\n  Gamma (ΔCS01 pour +1bp)   : "
              << std::setprecision(0) << dgamma << " EUR\n";
    if (std::abs(g.gamma) > 1e12)
        std::cout << "  ⚠  Instable numériquement\n";
    else if (dgamma > 0)
        std::cout << "  → Convexité positive : gains > pertes pour même mouvement\n";
    else
        std::cout << "  → Convexité négative : pertes > gains pour même mouvement\n";

    // --- Bucketed CS01 ---
    if (!g.bucketed_cs01.empty()) {
        std::cout << "\n  Bucketed CS01 (ΔNPV +1bp par pilier) :\n";
        double max_abs = 0;
        std::size_t dom = 0;
        for (std::size_t j = 0; j < g.bucketed_cs01.size(); ++j) {
            const double v = g.bucketed_cs01[j] * H_CS;
            if (std::abs(v) > max_abs) { max_abs = std::abs(v); dom = j; }
        }
        for (std::size_t j = 0; j < g.bucketed_cs01.size(); ++j) {
            const double v = g.bucketed_cs01[j] * H_CS;
            if (v == 0.0) continue;
            double pct = (dcs01 != 0.) ? v/dcs01*100. : 0.;
            std::cout << "    " << std::setw(4) << pillars[j] << "Y : "
                      << std::right << std::setw(12) << std::setprecision(0) << v
                      << " EUR  " << bar(v, max_abs*0.8, 12)
                      << "  (" << std::setprecision(1) << pct << "%)\n";
        }
        std::cout << "  → Pilier dominant : " << pillars[dom] << "Y ("
                  << std::setprecision(1)
                  << std::abs(g.bucketed_cs01[dom]*H_CS)/std::abs(dcs01)*100.
                  << "% du ΔNPV total)\n";
    }
}

// =============================================================================
//  main
// =============================================================================
int main() {

    banner("iTraxx Main S43 5Y — Risk Report   [Recursion, N=125, Notl=1Bn EUR]");

    const Core::Date ref(2026,5,11);

    // -------------------------------------------------------------------------
    //  1. Marché
    // -------------------------------------------------------------------------
    const auto yc  = build_yc(ref);
    const auto md  = build_cds_md(ref);
    const auto tmd = build_tranches_md(ref);

    Market::CreditBoot cb(md, yc, ref);
    const auto& cc = cb.curve();
    const std::vector<Market::CreditCurve> curves(N_CREDITS, cc);

    std::cout << "\n  Date de valorisation : 2026-05-11\n"
              << "  Index                : iTraxx Main Series 43  (5Y)\n"
              << "  Notionnel index      : " << INDEX_NOTL/1e6 << "M EUR\n"
              << "  N credits            : " << N_CREDITS << "\n"
              << "  Recovery rate        : " << RR*100 << "%\n"
              << "  Spread CDS 5Y index  : " << md.quotes[2].value*1e4 << " bp\n";

    // -------------------------------------------------------------------------
    //  2. Bootstrap base correlation
    // -------------------------------------------------------------------------
    banner("1. BASE CORRELATION BOOTSTRAP");

    Pricer::RecursionPricer rec_p(curves, N_CREDITS, RR);
    Pricer::BaseCorrelationBoot<Pricer::RecursionPricer> bc_boot(rec_p, yc);

    // Bootstrap sur les 5 premières tranches uniquement ([0-3%] à [12-22%])
    // Convention : on ne bootstrappe PAS le super-senior [22-100%]
    // La courbe BC s'arrête à K2=22% — rho(K>22%) = extrapolation flat depuis bc_curve.rho()
    const auto bc_res   = bc_boot.bootstrap(tmd);
    const auto& bc_curve = bc_res.curve;

    std::cout << "\n  " << std::left  << std::setw(14) << "Tranche"
              << std::right << std::setw(10) << "K2"
              << std::setw(14) << "rho calibré"
              << std::setw(16) << "Par spread (bp)"
              << std::setw(14) << "Résidu (EUR)"
              << "\n  " << std::string(68,'-') << "\n";

    const char* tnames[] = {"[0-3%]","[3-6%]","[6-9%]","[9-12%]","[12-22%]"};
    for (std::size_t i = 0; i < bc_curve.rho_curve_size(); ++i) {
        const auto& pt = bc_curve.rho_curve()[i];
        std::cout << "  " << std::left << std::setw(14) << tnames[i]
                  << std::right << std::fixed
                  << std::setw(10) << std::setprecision(2) << pt.K*100 << "%"
                  << std::setw(14) << std::setprecision(4) << pt.rho
                  << std::setw(16) << std::setprecision(2) << bc_res.par_spreads[i]*1e4
                  << std::setw(14) << std::setprecision(4) << bc_res.residuals[i]
                  << "\n";
    }
    // Super-senior : rho extrapolé flat depuis le dernier point (K2=22%)
    const double rho_ss = bc_curve.rho(0.22);   // extrapolation flat
    std::cout << "  " << std::left << std::setw(14) << "[22-100%]"
              << std::right << std::fixed
              << std::setw(10) << std::setprecision(2) << 100.0 << "%"
              << std::setw(14) << std::setprecision(4) << rho_ss
              << std::setw(16) << "N/A (extrap.)"
              << std::setw(14) << "N/A"
              << "\n";

    sub("Lecture");
    std::cout
        << "  La base correlation est le rho (copule gaussienne) qui annule le NPV\n"
        << "  de la base tranche [0-K] au spread coté. 5 points bootstrappés (K2=3%\n"
        << "  à K2=22%). Le super-senior utilise rho extrapolé flat depuis K2=22%.\n"
        << "  La courbe croissante → \'smile de corrélation\' (analogie vol implicite).\n";

    // -------------------------------------------------------------------------
    //  3. Par spreads avec rho bootstrappés
    // -------------------------------------------------------------------------
    banner("2. PAR SPREADS — rho de base correlation");

    std::vector<Market::Index_tranche> tranches;
    std::vector<Market::TranchesGrid>  grids;
    std::vector<double> rho1s, rho2s;

    for (int i = 0; i < N_TR; ++i) {
        const auto& ti = TINFO[i];
        Market::Index_tranche tr;
        tr.K1 = ti.K1; tr.K2 = ti.K2;
        tr.contractual_spread = ti.spd_bp * 1e-4;
        tr.fair_spread        = ti.spd_bp * 1e-4;
        tr.quoted_upfront     = ti.upfront;
        tr.upfront            = ti.uf_pct / 100.;
        tr.nominal            = (ti.K2 - ti.K1) * INDEX_NOTL;
        tr.maturity           = 5.;
        tr.effective_date     = ref;
        tr.valuation_date     = ref;
        tr.frequency          = Core::Frequency::QUARTERLY;
        tr.day_count          = Core::DayCount::ACT_360;
        tranches.push_back(tr);
        grids.push_back(Market::build_time_grid(tr, yc));

        // rho1 = rho(K1), rho2 = rho(K2) depuis la courbe bootstrappée
        const double r1 = (ti.K1 < 1e-12) ? bc_curve.rho(ti.K2) : bc_curve.rho(ti.K1);
        const double r2 = bc_curve.rho(ti.K2);
        rho1s.push_back(r1);
        rho2s.push_back(r2);
    }

    std::cout << "\n  " << std::left  << std::setw(14) << "Tranche"
              << std::right
              << std::setw(10) << "rho1"
              << std::setw(10) << "rho2"
              << std::setw(16) << "Par spread (bp)"
              << std::setw(14) << "Notionnel"
              << std::setw(14) << "RPV01 (EUR)"
              << "\n  " << std::string(78,'-') << "\n";

    for (int i = 0; i < N_TR; ++i) {
        const double par  = rec_p.par_spread(tranches[i], grids[i], rho1s[i], rho2s[i])*1e4;
        const double rpv  = rec_p.rpv01(tranches[i], grids[i], rho1s[i], rho2s[i]);
        const double notl_m = (TINFO[i].K2-TINFO[i].K1)*INDEX_NOTL/1e6;
        std::cout << "  " << std::left  << std::setw(14) << TINFO[i].name
                  << std::right << std::fixed
                  << std::setw(10) << std::setprecision(4) << rho1s[i]
                  << std::setw(10) << std::setprecision(4) << rho2s[i]
                  << std::setw(16) << std::setprecision(2) << par << " bp"
                  << std::setw(12) << std::setprecision(0) << notl_m << "M EUR"
                  << std::setw(14) << std::setprecision(0) << rpv
                  << "\n";
    }

    // -------------------------------------------------------------------------
    //  4. Greeks détaillés
    // -------------------------------------------------------------------------
    banner("3. GREEKS DÉTAILLÉS — Tranche par tranche");

    // MDInput = MultipleMarketData : un CDSMarketData par nom
    // Portfolio homogène ici → réplication du même md N fois
    // Sur un vrai desk : chaque entrée est le CDS md spécifique au nom i
    const Risk::MultipleMarketData mds(N_CREDITS, md);

    Risk::TranchesRiskEngine<Pricer::RecursionPricer> engine(
        rec_p, mds, yc, ref, N_CREDITS, Risk::Uniform_Recovery{RR}, "Recursion");

    Risk::TranchesBumpConfig cfg;
    cfg.compute_cs01=true; cfg.compute_dv01=true;
    cfg.compute_rho01=true; cfg.compute_bucketed_cs01=true;
    cfg.compute_gamma=true; cfg.compute_theta=false;
    cfg.use_central_diff=true;
    cfg.cs01_pillars={1.,3.,5.,7.,10.};

    std::cout << "\n  Calcul en cours (central diff)...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < N_TR; ++i) {
        const auto g = engine.compute_greeks(tranches[i], grids[i], rho1s[i], rho2s[i], cfg);
        print_greeks(g, i, cfg.cs01_pillars);
    }

    double ms = std::chrono::duration<double,std::milli>(
        std::chrono::high_resolution_clock::now()-t0).count();
    std::cout << "\n  [Greeks 6 tranches] " << std::fixed << std::setprecision(0) << ms << " ms\n";

    // -------------------------------------------------------------------------
    //  5. Book summary
    // -------------------------------------------------------------------------
    banner("4. BOOK SUMMARY");

    const auto book = engine.compute_book_greeks(tranches, grids, rho1s, rho2s, cfg);
    const double tot_cs01 = book.total_cs01();
    const double tot_dv01 = book.total_dv01();
    const double tot_rho  = book.total_rho01();

    std::cout << "\n  " << std::left << std::setw(14) << "Tranche"
              << std::right
              << std::setw(12) << "Spr (bp)"
              << std::setw(16) << "CS01 (EUR/bp)"
              << std::setw(16) << "DV01 (EUR/bp)"
              << std::setw(16) << "Rho01 (EUR/pt)"
              << std::setw(10) << "% CS01"
              << "\n  " << std::string(84,'-') << "\n";

    for (int i = 0; i < N_TR; ++i) {
        const auto& g = book.tranches[i];
        double pct = (tot_cs01!=0.) ? g.cs01/tot_cs01*100. : 0.;
        std::cout << "  " << std::left << std::setw(14) << TINFO[i].name
                  << std::right << std::fixed
                  << std::setw(12) << std::setprecision(1) << g.par_spread*1e4
                  << std::setw(16) << std::setprecision(0) << g.cs01
                  << std::setw(16) << g.dv01
                  << std::setw(16) << g.rho01
                  << std::setw(9)  << std::setprecision(1) << pct << "%"
                  << "\n";
    }
    std::cout << "  " << std::string(84,'-') << "\n"
              << "  " << std::left << std::setw(14) << "TOTAL"
              << std::right << std::fixed
              << std::setw(12) << ""
              << std::setw(16) << std::setprecision(0) << tot_cs01
              << std::setw(16) << tot_dv01
              << std::setw(16) << tot_rho
              << std::setw(9)  << "100.0%"
              << "\n\n";

    // Ordres de grandeur attendus
    std::cout << "  CS01 total  : " << tot_cs01/1e3 << "k EUR/bp\n"
              << "  DV01 total  : " << tot_dv01/1e3 << "k EUR/bp\n"
              << "  Rho01 total : " << tot_rho/1e3  << "k EUR/pt\n\n";

    std::cout << "  Règle de lecture :\n"
              << "  CS01 négatif → book LONG protection (acheteur de protection)\n"
              << "    Si spreads +10bp → P&L ≈ " << std::setprecision(0)
              << tot_cs01*10./1e6 << "M EUR\n"
              << "  Rho01 "<<(tot_rho>0?"positif":"négatif")<<" → book net "
              << (tot_rho>0?"LONG":"COURT") << " corrélation\n";

    // -------------------------------------------------------------------------
    //  6. Smile de corrélation
    // -------------------------------------------------------------------------
    banner("5. SMILE DE CORRÉLATION — bootstrappé vs flat");

    std::cout << "\n  " << std::left << std::setw(14) << "Tranche"
              << std::right
              << std::setw(10) << "rho(K2)"
              << std::setw(16) << "spd(rho BC)"
              << std::setw(16) << "spd(flat 0.30)"
              << std::setw(14) << "Δ (bp)"
              << "\n  " << std::string(70,'-') << "\n";

    const double rho_flat = 0.30;
    for (int i = 0; i < N_TR; ++i) {
        const double s_bc  = rec_p.par_spread(tranches[i],grids[i],rho1s[i],rho2s[i])*1e4;
        const double s_fl  = rec_p.par_spread(tranches[i],grids[i],rho_flat,rho_flat)*1e4;
        std::cout << "  " << std::left << std::setw(14) << TINFO[i].name
                  << std::right << std::fixed
                  << std::setw(10) << std::setprecision(4) << rho2s[i]
                  << std::setw(16) << std::setprecision(2) << s_bc << " bp"
                  << std::setw(16) << std::setprecision(2) << s_fl << " bp"
                  << std::setw(14) << std::setprecision(2) << (s_bc-s_fl)
                  << "\n";
    }
    sub("Lecture");
    std::cout
        << "  La base correlation bootstrappée reproduit exactement les cotations marché.\n"
        << "  Un rho flat à 0.30 sur-évalue les tranches equity (rho trop haut)\n"
        << "  et sous-évalue les tranches senior (rho trop bas).\n"
        << "  Le smile est la 'volatilité implicite du crédit corrélé'.\n";

    banner("Fin du rapport");
    std::cout << "\n";
    return 0;
}