# CDS & Index Tranches Pricing

> A professional grade C++20 library for pricing and risk management of single-name CDS and index tranche products, with Python bindings via pybind11.



---

## Overview

This library implements a full pricing stack for credit derivatives, from market data bootstrapping down to instrument valuation. It covers:

- **Yield curve construction** from deposits, Eurodollar futures (with convexity adjustment), and vanilla IRS, bootstrapped via Brent's method
- **Credit curve bootstrapping** from CDS par spreads, using Newton-Raphson with analytical Jacobian
- **CDS pricing**  par spread, risky PV01, NPV, upfront  with support for mid-life valuation
- **Python bindings** exposing the full C++ API via pybind11

The architecture enforces a strict separation between market data, instruments, numerical solvers, and pricing engines.

---

## Project Structure

```
CDS and IndexTranches Pricing/
│
├── Core/
│   ├── Dates.hpp / .cpp        # Julian day arithmetic, calendar operations
│   ├── interpolator.hpp / .cpp # LogLinearInterpolator, LinearInterpolator
│   ├── numericals.hpp          # NewtonRaphson<F>, Brent<F> — templated solvers
│   └── types.hpp               # Point, DayCount (ACT/360, ACT/365, 30/360), Frequency
│
├── Market/
│   ├── instruments.hpp / .cpp  # Deposit, Future, Swap, CDS, CDSMarketData
│   ├── YieldCurve.hpp / .cpp   # YieldCurve + YieldCurveBoot (Brent bootstrapper)
│   └── CreditCurve.hpp / .cpp  # CreditCurve + CreditBoot (Newton-Raphson bootstrapper)
│
├── Pricers/
│   └── pricers.hpp / .cpp      # CDSPricer: par_spread, rpv01, npv, upfront
│
├── bindings/
│   ├── bindings.cpp            # pybind11 module — exposes full C++ API to Python
│   ├── CMakeLists.txt          # Build config for the Python extension
│   ├── example_usage.py        # Full Python usage demo
│   └── README_bindings.md      # Bindings build instructions
│
├── main.cpp                    # C++ demo: yield curve + credit curve + CDS pricing
└── CMakeLists.txt              # Top-level build config (C++ exe + optional Python module)
```
## Build

### Requirements

- C++20 compiler (GCC 11+, Clang 13+, MSVC 2022)
- CMake ≥ 4.1
- pybind11 (optional — required only for Python bindings)

### C++ executable

```bash
git clone https://github.com/<your-handle>/cds-pricing.git
cd cds-pricing

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/CDS_and_IndexTranches_Pricing
```

### Python bindings

```bash
pip install pybind11

cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)
cmake --build build

# On Linux/macOS: build/cds_pricer.so
# On Windows:     build/cds_pricer.cp312-win_amd64.pyd
```

---

## Usage

### C++

```cpp
#include "Core/Dates.hpp"
#include "Market/YieldCurve.hpp"
#include "Market/CreditCurve.hpp"
#include "Market/instruments.hpp"
#include "Pricers/pricers.hpp"

const Core::Date today(2026, 3, 26);

// 1. Yield curve
Market::YieldCurveBoot boot(today);
boot.add_deposits({ {3.0/12, 0.0400} });
boot.add_swaps(   { {5.0, 0.0345, 0.25, 0.25} });
const auto& yc = boot.curve();

// 2. Credit curve
Market::CDSMarketData mdata;
mdata.name          = "ACME";
mdata.effectiveDate = today;
mdata.valuationDate = today;
mdata.recoveryRate  = 0.40;
mdata.frequency     = Core::Frequency::QUARTERLY;
mdata.quotes        = { {1.0, 0.0050}, {3.0, 0.0100}, {5.0, 0.0130} };

Market::CreditBoot cc_boot(mdata, yc, today);
const auto& cc = cc_boot.curve();

// 3. CDS pricer
Market::CDS cds;
cds.maturity          = 5.0;
cds.Nominal           = 10'000'000.0;
cds.EffectiveDate     = today;
cds.ValuationDate     = today;
cds.RecoveryRate      = 0.40;
cds.frequency         = Core::Frequency::QUARTERLY;
cds.ContractualSpread = 0.0100;  // 100bp

Pricer::CDSPricer pricer(cds, yc, cc);
std::cout << "Par spread : " << pricer.par_spread() * 1e4 << " bp\n";
std::cout << "RPV01      : " << pricer.rpv01()            << "\n";
std::cout << "NPV        : " << pricer.npv()              << "\n";
std::cout << "Upfront    : " << pricer.upfront()          << "\n";
```

### Python

```python
import cds_pricer as cds

today = cds.Date(2026, 3, 26)

# Yield curve
boot = cds.YieldCurveBoot(today)
boot.add_deposits([cds.Deposit(3.0/12, 0.0400)])
boot.add_swaps([cds.Swap(5.0, 0.0345, 0.25, 0.25)])
yc = boot.curve()

# Credit curve
mdata = cds.CDSMarketData()
mdata.name           = "ACME"
mdata.effective_date = today
mdata.valuation_date = today
mdata.recovery_rate  = 0.40
mdata.frequency      = cds.Frequency.QUARTERLY
mdata.quotes         = [cds.Point(1.0, 0.005), cds.Point(5.0, 0.013)]

cc_boot = cds.CreditBoot(mdata, yc, today)
cc = cc_boot.curve()

# CDS pricer
instrument = cds.CDS()
instrument.maturity           = 5.0
instrument.nominal            = 10_000_000.0
instrument.effective_date     = today
instrument.valuation_date     = today
instrument.recovery_rate      = 0.40
instrument.frequency          = cds.Frequency.QUARTERLY
instrument.contractual_spread = 0.0100

pricer = cds.CDSPricer(instrument, yc, cc)
print(f"Par spread : {pricer.par_spread() * 1e4:.4f} bp")
print(f"RPV01      : {pricer.rpv01():.6f}")
print(f"NPV        : {pricer.npv():.2f}")
print(f"Upfront    : {pricer.upfront():.6f}")
```

> **Important — object lifetimes (Python):** `YieldCurve` and `CreditCurve` are returned as internal references from their respective bootstrappers. The bootstrapper objects (`boot`, `cc_boot`) must remain alive as long as any curve or pricer that depends on them is in use.

--

## Roadmap

| Status | Feature |
|--------|---------|
| ✅ | Yield curve bootstrapper — deposits, futures (convexity adj.), IRS (Brent) |
| ✅ | Credit curve bootstrapper — Newton-Raphson with analytical Jacobian |
| ✅ | CDS pricer — par spread, RPV01, NPV, upfront, mid-life valuation |
| ✅ | Python bindings (pybind11) |
| 📋 | CS01 / IR DV01 via central finite differences |
| 📋 | Index CDS (CDX / iTraxx) intrinsic pricing |
| 📋 | Tranche pricing (Gaussian copula, base correlation) |
| 📋 | CVA on single-name CDS |

---

## Design Notes

- **No external dependencies** beyond the STL and pybind11 (for the Python module).
- **No polymorphism** unless architecturally necessary — concrete types and templates throughout.
- **`Core::Date`** uses Julian day arithmetic as its internal representation; all date differences and year fractions are computed directly from Julian day integers.
- **Solvers are stateless value types** — `NewtonRaphson` and `Brent` take callables via templates, with no heap allocation.
- **`friend class CreditBoot`** grants the bootstrapper direct write access to `CreditCurve::intensity` without exposing mutation publicly.
- **`YieldCurveBoot::solve_df`** uses `pop_pillar` to implement a try/reject pattern inside Brent without cloning the curve.

---

## References

- O'Kane, D. (2008). *Modelling Single-name and Multi-name Credit Derivatives.* Wiley.
