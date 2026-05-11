# README — Bindings Python pour CDS & IndexTranches Pricing

## Structure des fichiers

```
ton_projet/
├── CDS_src/                        ← ton code C++ original (copie ou symlink)
│   ├── Core/
│   │   ├── Dates.hpp / .cpp
│   │   ├── interpolator.hpp / .cpp
│   │   ├── numericals.hpp
│   │   └── types.hpp
│   ├── Market/
│   │   ├── instruments.hpp / .cpp
│   │   ├── YieldCurve.hpp / .cpp
│   │   └── CreditCurve.hpp / .cpp
│   └── Pricers/
│       └── pricers.hpp / .cpp
│
├── bindings/                       ← fichiers fournis ici
│   ├── bindings.cpp                ← le binding pybind11
│   ├── CMakeLists.txt              ← build du module Python
│   ├── example_usage.py            ← exemple Python complet
│   └── README_bindings.md          ← ce fichier
```

---

## Prérequis

### Linux / macOS
```bash
sudo apt install cmake python3-dev   # ou brew install cmake
pip install pybind11
```

### Windows (MSVC)
```powershell
pip install pybind11
# Visual Studio 2022 avec "C++ CMake tools" suffit
```

---

## Compilation

### Étape 1 — Organiser les sources

Copie (ou crée un symlink) de ton dossier C++ vers `CDS_src/` :
```bash
cp -r "CDS and IndexTranches Pricing" CDS_src
```

### Étape 2 — Configurer et compiler

```bash
cd bindings
mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)

cmake --build . --config Release
```

Le résultat est un fichier `cds_pricer.so` (Linux/macOS) ou `cds_pricer.pyd` (Windows).

### Étape 3 — Utiliser depuis Python

```bash
# depuis le dossier build/ ou après avoir copié le .so
python3 example_usage.py
```

Ou dans ton script :
```python
import cds_pricer as cds
```

---

## API Python — Référence rapide

### Core

```python
# Date
d = cds.Date(2026, 3, 26)
d.add_days(90)
d.add_months(6)
d.get_julian_days()
d2 - d1          # → int (nb de jours)

cds.year_fraction(d1, d2, cds.DayCount.ACT_365)
cds.year_fraction(d1, d2, cds.DayCount.ACT_360)
cds.year_fraction(d1, d2, cds.DayCount.DC_30_360)

# Point(time, value)
p = cds.Point(5.0, 0.0130)

# Enums
cds.Frequency.QUARTERLY
cds.Frequency.SEMI_ANNUAL
cds.Frequency.ANNUAL
```

### Yield Curve

```python
boot = cds.YieldCurveBoot(today)
boot.add_deposits([cds.Deposit(maturity, rate), ...])
boot.add_futures([cds.Future(eff_date, mat_date, price, vol), ...])
boot.add_swaps([cds.Swap(maturity, fixed_rate, fixed_freq, float_freq), ...])
yc = boot.curve()

yc.discount(t)            # facteur d'actualisation
yc.forward_rate(t1, t2)   # taux forward continu
yc.num_pillars()
```

### Credit Curve

```python
mdata = cds.CDSMarketData()
mdata.name           = "ACME_5Y"
mdata.effective_date = today
mdata.valuation_date = today
mdata.recovery_rate  = 0.40
mdata.frequency      = cds.Frequency.QUARTERLY
mdata.quotes         = [cds.Point(t, spread), ...]   # spread en décimal

cc_boot = cds.CreditBoot(mdata, yc, today)
cc = cc_boot.curve()

cc.survival_probability(t)      # Q(0,t)
cc.survival_probability(t1, t2) # Q(t1,t2) conditionnel
```

### CDS Pricer

```python
instrument = cds.CDS()
instrument.name               = "ACME_5Y"
instrument.maturity           = 5.0          # années
instrument.nominal            = 10_000_000.0
instrument.effective_date     = today
instrument.valuation_date     = today
instrument.recovery_rate      = 0.40
instrument.frequency          = cds.Frequency.QUARTERLY
instrument.contractual_spread = 0.0100       # 100bp en décimal

pricer = cds.CDSPricer(instrument, yc, cc)

pricer.par_spread()          # spread par en décimal (×1e4 pour bp)
pricer.rpv01()               # risky PV01
pricer.npv()                 # NPV avec le spread contractuel
pricer.npv(0.0120)           # NPV avec un spread différent
pricer.upfront()             # upfront en fraction du nominal
pricer.upfront(0.0120)
```

---

## Durée de vie des objets — point important

Les objets `YieldCurve` et `CreditCurve` sont retournés **par référence interne**
de `YieldCurveBoot` et `CreditBoot`. Il faut garder le bootstrapper **vivant**
aussi longtemps que la courbe est utilisée :

```python
# ✅ Correct
boot  = cds.YieldCurveBoot(today)
# ... add instruments ...
yc    = boot.curve()           # référence interne à boot
pricer = cds.CDSPricer(instr, yc, cc)   # OK tant que boot et cc_boot sont vivants

# ❌ Incorrect — boot détruit avant pricer
yc = cds.YieldCurveBoot(today).curve()  # dangling reference !
```
