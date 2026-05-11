"""
example_usage.py
================
Démonstration complète des bindings Python pour la bibliothèque
CDS & IndexTranches Pricing.

Prérequis :
    - Avoir compilé le module cds_pricer.so (voir README_bindings.md)
    - Le fichier cds_pricer.so doit être dans le même répertoire (ou dans PYTHONPATH)
"""

import sys
import cds_pricer as cds   

# ─────────────────────────────────────────────────────────────────────────────
# 1. Dates
# ─────────────────────────────────────────────────────────────────────────────
today = cds.Date(2026, 3, 26)
print(f"Today          : {today}")                         # Date(2026, 3, 26)
print(f"Today + 90j    : {today.add_days(90)}")
print(f"Today + 6 mois : {today.add_months(6)}")
print(f"Julian day     : {today.get_julian_days()}")

d1 = cds.Date(2026, 3, 26)
d2 = cds.Date(2027, 3, 26)
print(f"Jours entre d1/d2 : {d2 - d1}")
print(f"YF ACT/365        : {cds.year_fraction(d1, d2, cds.DayCount.ACT_365):.6f}")
print()

# ─────────────────────────────────────────────────────────────────────────────
# 2. Bootstrap de la courbe de taux (YieldCurve)
# ─────────────────────────────────────────────────────────────────────────────
print("=== Yield Curve Bootstrap ===")

boot = cds.YieldCurveBoot(today)

# Dépôts
boot.add_deposits([
    cds.Deposit(1.0/365,  0.0390),
    cds.Deposit(7.0/365,  0.0392),
    cds.Deposit(1.0/12,   0.0395),
    cds.Deposit(3.0/12,   0.0400),
])

# Futures Eurodollar
boot.add_futures([
    cds.Future(3.0/12,   6.0/12,  95.90, 0.01),
    cds.Future(6.0/12,   9.0/12,  96.00, 0.01),
    cds.Future(9.0/12,  12.0/12,  96.15, 0.01),
    cds.Future(12.0/12, 15.0/12,  96.30, 0.01),
])

# Swaps
boot.add_swaps([
    cds.Swap( 2.0,  0.0470, 0.25, 0.25),
    cds.Swap( 3.0,  0.0360, 0.25, 0.25),
    cds.Swap( 5.0,  0.0345, 0.25, 0.25),
    cds.Swap(10.0,  0.0338, 0.25, 0.25),
])

yc = boot.curve()

print(f"{'t':>6}  {'DF':>12}")
print("-" * 22)
for t in [0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0]:
    print(f"{t:>6.2f}  {yc.discount(t):>12.6f}")
print()

# ─────────────────────────────────────────────────────────────────────────────
# 3. Bootstrap de la courbe de crédit (CreditCurve)
# ─────────────────────────────────────────────────────────────────────────────
print("=== Credit Curve Bootstrap ===")

mdata = cds.CDSMarketData()
mdata.name           = "ACME_5Y"
mdata.effective_date = today
mdata.valuation_date = today
mdata.recovery_rate  = 0.40
mdata.frequency      = cds.Frequency.QUARTERLY
mdata.quotes = [
    cds.Point(1.0,   0.0050),
    cds.Point(2.0,   0.0080),
    cds.Point(3.0,   0.0100),
    cds.Point(5.0,   0.0130),
    cds.Point(7.0,   0.0150),
    cds.Point(10.0,  0.0170),
]

cc_boot = cds.CreditBoot(mdata, yc, today)
cc = cc_boot.curve()

print(f"{'t':>6}  {'Q(t)':>10}  {'PD (%)':>10}")
print("-" * 32)
for t in [1.0, 2.0, 3.0, 5.0, 7.0, 10.0]:
    q = cc.survival_probability(t)
    print(f"{t:>6.1f}  {q:>10.6f}  {(1 - q)*100:>10.2f}")
print()

# ─────────────────────────────────────────────────────────────────────────────
# 4. Vérification de la calibration (repricing)
# ─────────────────────────────────────────────────────────────────────────────
print("=== Repricing Check ===")
print(f"{'Maturity':>10}  {'Quoted (bp)':>12}  {'Repriced (bp)':>14}")
print("-" * 42)

for pt in mdata.quotes:
    # Construire un CDS temporaire pour extraire le par spread
    tmp = cds.CDS()
    tmp.maturity          = pt.time
    tmp.recovery_rate     = mdata.recovery_rate
    tmp.frequency         = mdata.frequency
    tmp.effective_date    = mdata.effective_date
    tmp.valuation_date    = mdata.valuation_date
    tmp.contractual_spread = 0.0
    tmp.nominal           = 1_000_000.0

    pricer_tmp = cds.CDSPricer(tmp, yc, cc)
    repriced   = pricer_tmp.par_spread()
    print(f"{pt.time:>10.1f}  {pt.value*1e4:>12.2f}  {repriced*1e4:>14.2f}")

print()

# ─────────────────────────────────────────────────────────────────────────────
# 5. Pricing d'un CDS
# ─────────────────────────────────────────────────────────────────────────────
print("=== CDS Pricer ===")

instrument = cds.CDS()
instrument.name               = "ACME_5Y"
instrument.maturity           = 5.0
instrument.nominal            = 10_000_000.0        # 10 M
instrument.effective_date     = today
instrument.valuation_date     = today
instrument.recovery_rate      = 0.40
instrument.frequency          = cds.Frequency.QUARTERLY
instrument.contractual_spread = 0.0100              # 100 bp

pricer = cds.CDSPricer(instrument, yc, cc)

print(f"  Par spread (bp) : {pricer.par_spread() * 1e4:>10.4f}")
print(f"  RPV01           : {pricer.rpv01():>10.6f}")
print(f"  NPV             : {pricer.npv():>10.2f}")
print(f"  Upfront         : {pricer.upfront():>10.6f}")
print()

# ─────────────────────────────────────────────────────────────────────────────
# 6. Sensibilités simples (bump & reprice)
# ─────────────────────────────────────────────────────────────────────────────
print("=== Sensibilités ===")

base_npv = pricer.npv()

# CS01 : bump du spread contractuel de +1 bp
instrument2 = cds.CDS()
instrument2.name               = "ACME_5Y"
instrument2.maturity           = 5.0
instrument2.nominal            = 10_000_000.0
instrument2.effective_date     = today
instrument2.valuation_date     = today
instrument2.recovery_rate      = 0.40
instrument2.frequency          = cds.Frequency.QUARTERLY
instrument2.contractual_spread = 0.0100 + 1e-4      # +1 bp

pricer2 = cds.CDSPricer(instrument2, yc, cc)
cs01 = pricer2.npv() - base_npv
print(f"  CS01 (+1bp contractual spread) : {cs01:>10.2f} USD")

# Survival proba conditionnelle (exemple)
q_1_5 = cc.survival_probability(1.0, 5.0)
print(f"  Q(1Y → 5Y) : {q_1_5:.6f}")
