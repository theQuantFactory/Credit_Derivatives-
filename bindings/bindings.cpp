// bindings.cpp
// pybind11 binding — expose CDS & IndexTranches Pricing library to Python
// Covers: Core::Date, Core::DayCount, Core::Frequency
//         Market::Deposit, Future, Swap, CDS, CDSMarketData
//         Market::YieldCurve, YieldCurveBoot
//         Market::CreditCurve, CreditBoot
//         Pricer::CDSPricer

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>       // std::vector, std::string auto-conversion

#include "Core/Dates.hpp"
#include "Core/types.hpp"
#include "Core/numericals.hpp"
#include "Market/instruments.hpp"
#include "Market/YieldCurve.hpp"
#include "Market/CreditCurve.hpp"
#include "Pricers/pricers.hpp"

namespace py = pybind11;

PYBIND11_MODULE(cds_pricer, m) {

    m.doc() = "CDS & IndexTranches Pricing — pybind11 bindings";

    // ---------------------------------------------------------------
    // Core::Date
    // ---------------------------------------------------------------
    py::class_<Core::Date>(m, "Date")
        .def(py::init<int, int, int>(), py::arg("year"), py::arg("month"), py::arg("day"),
             "Construct a Date from year / month / day.")
        .def(py::init<>(),
             "Construct today's date.")
        .def("get_julian_days", &Core::Date::getJulianDays,
             "Return the internal Julian day count.")
        .def("add_days",   &Core::Date::add_days,   py::arg("days"),
             "Return a new Date shifted by *days* calendar days.")
        .def("add_months",
             static_cast<Core::Date (Core::Date::*)(int) const>(&Core::Date::add_months),
             py::arg("n_months"),
             "Return a new Date shifted by *n_months* whole months.")
        .def("__sub__",
             [](const Core::Date& a, const Core::Date& b){ return a - b; },
             py::arg("other"), "Number of calendar days between two dates.")
        .def("__lt__",  &Core::Date::operator<)
        .def("__le__",  &Core::Date::operator<=)
        .def("__gt__",  &Core::Date::operator>)
        .def("__ge__",  &Core::Date::operator>=)
        .def("__eq__",  &Core::Date::operator==)
        .def("__ne__",  &Core::Date::operator!=)
        .def("__repr__",
             [](const Core::Date& d){
                 auto [y,mo,da] = Core::Date::fromJulian(d.getJulianDays());
                 return "Date(" + std::to_string(y) + ", " +
                                  std::to_string(mo) + ", " +
                                  std::to_string(da) + ")";
             })
        .def_static("from_julian", &Core::Date::fromJulian, py::arg("julian_day"),
             "Return (year, month, day) tuple from a Julian day number.");

    // ---------------------------------------------------------------
    // Core::DayCount
    // ---------------------------------------------------------------
    py::enum_<Core::DayCount>(m, "DayCount")
        .value("ACT_360",   Core::DayCount::ACT_360)
        .value("ACT_365",   Core::DayCount::ACT_365)
        .value("DC_30_360", Core::DayCount::DC_30_360)
        .export_values();

    // ---------------------------------------------------------------
    // Core::Frequency
    // ---------------------------------------------------------------
    py::enum_<Core::Frequency>(m, "Frequency")
        .value("QUARTERLY",   Core::Frequency::QUARTERLY)
        .value("SEMI_ANNUAL", Core::Frequency::SEMI_ANNUAL)
        .value("ANNUAL",      Core::Frequency::ANNUAL)
        .export_values();

    // ---------------------------------------------------------------
    // Core::Point
    // ---------------------------------------------------------------
    py::class_<Core::Point>(m, "Point")
        .def(py::init<>())
        .def(py::init([](double t, double v){
            Core::Point p; p.time = t; p.value = v; return p;
        }), py::arg("time"), py::arg("value"))
        .def_readwrite("time",  &Core::Point::time)
        .def_readwrite("value", &Core::Point::value)
        .def("__repr__", [](const Core::Point& p){
            return "Point(time=" + std::to_string(p.time) +
                   ", value=" + std::to_string(p.value) + ")";
        });

    // ---------------------------------------------------------------
    // Free function: year_fraction
    // ---------------------------------------------------------------
    m.def("year_fraction", &Core::year_fraction,
          py::arg("d1"), py::arg("d2"), py::arg("day_count"),
          "Compute the year fraction between two dates using the given day-count convention.");

    // ---------------------------------------------------------------
    // Market::Deposit
    // ---------------------------------------------------------------
    py::class_<Market::Deposit>(m, "Deposit")
        .def(py::init<>())
        .def(py::init([](double maturity, double rate){
            Market::Deposit d; d.maturity = maturity; d.rate = rate; return d;
        }), py::arg("maturity"), py::arg("rate"))
        .def_readwrite("maturity",   &Market::Deposit::maturity)
        .def_readwrite("rate",       &Market::Deposit::rate)
        .def("implied_df",           &Market::Deposit::implied_df,
             "Return the discount factor implied by this deposit.");

    // ---------------------------------------------------------------
    // Market::Future
    // ---------------------------------------------------------------
    py::class_<Market::Future>(m, "Future")
        .def(py::init<>())
        .def(py::init([](double eff, double mat, double price, double vol){
            Market::Future f;
            f.effective_date = eff; f.maturity_date = mat;
            f.price = price;       f.volatility = vol;
            return f;
        }), py::arg("effective_date"), py::arg("maturity_date"),
            py::arg("price"), py::arg("volatility"))
        .def_readwrite("effective_date", &Market::Future::effective_date)
        .def_readwrite("maturity_date",  &Market::Future::maturity_date)
        .def_readwrite("price",          &Market::Future::price)
        .def_readwrite("volatility",     &Market::Future::volatility)
        .def("fra_market_rate", &Market::Future::fra_market_rate,
             "Return the convexity-adjusted FRA rate.")
        .def("npv", &Market::Future::npv, py::arg("yield_curve"),
             "NPV of the future versus the curve.");

    // ---------------------------------------------------------------
    // Market::Swap::Schedule (inner class)
    // ---------------------------------------------------------------
    py::class_<Market::Swap::Schedule>(m, "SwapSchedule")
        .def(py::init<>())
        .def_readwrite("fixed",    &Market::Swap::Schedule::fixed)
        .def_readwrite("floating", &Market::Swap::Schedule::floating);

    // ---------------------------------------------------------------
    // Market::Swap
    // ---------------------------------------------------------------
    py::class_<Market::Swap>(m, "Swap")
        .def(py::init<>())
        .def(py::init([](double mat, double rate, double ff, double flt){
            Market::Swap s;
            s.maturity = mat; s.fixedRate = rate;
            s.fixedLeg_freq = ff; s.floatLeg_freq = flt;
            return s;
        }), py::arg("maturity"), py::arg("fixed_rate"),
            py::arg("fixed_freq"), py::arg("float_freq"))
        .def_readwrite("maturity",      &Market::Swap::maturity)
        .def_readwrite("fixed_rate",    &Market::Swap::fixedRate)
        .def_readwrite("fixed_freq",    &Market::Swap::fixedLeg_freq)
        .def_readwrite("float_freq",    &Market::Swap::floatLeg_freq)
        .def("build_schedule", &Market::Swap::buildSchedule,
             "Generate fixed and floating date grids.")
        .def("npv", &Market::Swap::npv,
             py::arg("yield_curve"), py::arg("schedule"),
             "NPV of the swap (fixed minus floating PV).");

    // ---------------------------------------------------------------
    // Market::CDSMarketData
    // ---------------------------------------------------------------
    py::class_<Market::CDSMarketData>(m, "CDSMarketData")
        .def(py::init<>())
        .def_readwrite("name",           &Market::CDSMarketData::name)
        .def_readwrite("effective_date", &Market::CDSMarketData::effectiveDate)
        .def_readwrite("valuation_date", &Market::CDSMarketData::valuationDate)
        .def_readwrite("recovery_rate",  &Market::CDSMarketData::recoveryRate)
        .def_readwrite("frequency",      &Market::CDSMarketData::frequency)
        .def_readwrite("quotes",         &Market::CDSMarketData::quotes,
             "List of Point(time, spread) market quotes used for bootstrapping.");

    // ---------------------------------------------------------------
    // Market::CDS::CDSGrids (inner class)
    // ---------------------------------------------------------------
    py::class_<Market::CDS::CDSGrids>(m, "CDSGrids")
        .def(py::init<>())
        .def_readwrite("default_times", &Market::CDS::CDSGrids::defaultTimes)
        .def_readwrite("premium_times", &Market::CDS::CDSGrids::premiumTimes);

    // ---------------------------------------------------------------
    // Market::CDS
    // ---------------------------------------------------------------
    py::class_<Market::CDS>(m, "CDS")
        .def(py::init<>())
        .def_readwrite("name",               &Market::CDS::Name)
        .def_readwrite("effective_date",     &Market::CDS::EffectiveDate)
        .def_readwrite("valuation_date",     &Market::CDS::ValuationDate)
        .def_readwrite("maturity",           &Market::CDS::maturity)
        .def_readwrite("nominal",            &Market::CDS::Nominal)
        .def_readwrite("frequency",          &Market::CDS::frequency)
        .def_readwrite("contractual_spread", &Market::CDS::ContractualSpread)
        .def_readwrite("recovery_rate",      &Market::CDS::RecoveryRate)
        .def("get_frequency",    &Market::CDS::get_frequency,
             "Return the payment frequency in months (3, 6 or 12).")
        .def("get_maturity_date",&Market::CDS::get_MaturityDate,
             "Return the maturity date as a Core::Date.")
        .def("build_cds_grids",  &Market::CDS::buildCDSGrids,
             py::arg("default_freq") = 1,
             "Build default-leg and premium-leg date grids.");

    // ---------------------------------------------------------------
    // Market::YieldCurve
    // ---------------------------------------------------------------
    py::class_<Market::YieldCurve>(m, "YieldCurve")
        .def(py::init<Core::Date>(), py::arg("ref_date"),
             "Construct an empty yield curve with the given reference date.")
        .def("add_pillar",    &Market::YieldCurve::add_pillar,
             py::arg("t"), py::arg("df"),
             "Manually add a discount-factor pillar at time t.")
        .def("discount",      &Market::YieldCurve::discount,
             py::arg("t"), "Return the discount factor at time t (log-linear interp).")
        .def("forward_rate",  &Market::YieldCurve::forward_rate,
             py::arg("t1"), py::arg("t2"),
             "Return the continuously-compounded forward rate between t1 and t2.")
        .def("num_pillars",   &Market::YieldCurve::num_pillars,
             "Return the number of pillars currently in the curve.");

    // ---------------------------------------------------------------
    // Market::YieldCurveBoot
    // ---------------------------------------------------------------
    py::class_<Market::YieldCurveBoot>(m, "YieldCurveBoot")
        .def(py::init<Core::Date>(), py::arg("ref_date"),
             "Create a bootstrapper for the given reference date.")
        .def("add_deposit",  py::overload_cast<const Market::Deposit&>(
                                &Market::YieldCurveBoot::add_deposit),
             py::arg("deposit"), "Add one deposit instrument.")
        .def("add_deposits", &Market::YieldCurveBoot::add_deposits,
             py::arg("deposits"), "Add a list of deposits.")
        .def("add_future",   py::overload_cast<const Market::Future&>(
                                &Market::YieldCurveBoot::add_future),
             py::arg("future"), "Add one Eurodollar future.")
        .def("add_futures",  &Market::YieldCurveBoot::add_futures,
             py::arg("futures"), "Add a list of futures.")
        .def("add_swap",     py::overload_cast<const Market::Swap&>(
                                &Market::YieldCurveBoot::add_swap),
             py::arg("swap"), "Add one par swap.")
        .def("add_swaps",    &Market::YieldCurveBoot::add_swaps,
             py::arg("swaps"), "Add a list of par swaps.")
        .def("curve",        &Market::YieldCurveBoot::curve,
             py::return_value_policy::reference_internal,
             "Return the bootstrapped YieldCurve (valid as long as this object is alive).");

    // ---------------------------------------------------------------
    // Market::CreditCurve
    // ---------------------------------------------------------------
    py::class_<Market::CreditCurve>(m, "CreditCurve")
        .def("survival_probability",
             py::overload_cast<double>(&Market::CreditCurve::survival_probability, py::const_),
             py::arg("t"), "Q(t) — survival probability from 0 to t.")
        .def("survival_probability",
             py::overload_cast<double,double>(&Market::CreditCurve::survival_probability, py::const_),
             py::arg("t"), py::arg("T"),
             "Q(t,T) — conditional survival probability from t to T.");

    // ---------------------------------------------------------------
    // Market::CreditBoot
    // ---------------------------------------------------------------
    py::class_<Market::CreditBoot>(m, "CreditBoot")
        .def(py::init<Market::CDSMarketData, const Market::YieldCurve&, Core::Date>(),
             py::arg("market_data"), py::arg("yield_curve"), py::arg("ref_date"),
             "Bootstrap a credit (hazard-rate) curve from CDS market quotes.\n"
             "Uses Newton-Raphson pillar by pillar.")
        .def("curve", &Market::CreditBoot::curve,
             py::return_value_policy::reference_internal,
             "Return the bootstrapped CreditCurve (valid as long as this object is alive).");

    // ---------------------------------------------------------------
    // Pricer::CDSPricer
    // ---------------------------------------------------------------
    py::class_<Pricer::CDSPricer>(m, "CDSPricer")
        .def(py::init<const Market::CDS&, const Market::YieldCurve&, const Market::CreditCurve&>(),
             py::arg("cds"), py::arg("yield_curve"), py::arg("credit_curve"),
             "Construct a CDS pricer.  All three references must outlive this object.")
        .def("par_spread", &Pricer::CDSPricer::par_spread,
             "Return the par spread (decimal). Multiply by 1e4 for basis points.")
        .def("rpv01",      &Pricer::CDSPricer::rpv01,
             "Return the risky PV01 (duration of the premium leg).")
        .def("npv",
             py::overload_cast<>(&Pricer::CDSPricer::npv, py::const_),
             "NPV using the contractual spread stored in the CDS object.")
        .def("npv",
             py::overload_cast<double>(&Pricer::CDSPricer::npv, py::const_),
             py::arg("actual_spread"),
             "NPV using a given actual spread (decimal).")
        .def("upfront",
             py::overload_cast<>(&Pricer::CDSPricer::upfront, py::const_),
             "Upfront payment (as fraction of nominal) with contractual spread.")
        .def("upfront",
             py::overload_cast<double>(&Pricer::CDSPricer::upfront, py::const_),
             py::arg("actual_spread"),
             "Upfront payment using a given actual spread (decimal).");
}
