<<<<<<< HEAD
cmake_minimum_required(VERSION 4.1)
project(CDS_and_IndexTranches_Pricing)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(CDS_and_IndexTranches_Pricing main.cpp
        Core/Dates.cpp
        Core/Dates.hpp
        Core/interpolator.cpp
        Core/interpolator.hpp
        Core/numericals.hpp
        Market/instruments.cpp
        Market/instruments.hpp
        Core/types.hpp
        Market/YieldCurve.cpp
        Market/YieldCurve.hpp
        Market/CreditCurve.cpp
        Market/CreditCurve.hpp
        Pricers/pricers.cpp
        Pricers/pricers.hpp
)

find_package(pybind11 QUIET)
if(pybind11_FOUND)
    pybind11_add_module(cds_pricer
            bindings/bindings.cpp
            Core/Dates.cpp
            Core/interpolator.cpp
            Market/instruments.cpp
            Market/YieldCurve.cpp
            Market/CreditCurve.cpp
            Pricers/pricers.cpp
    )
    target_include_directories(cds_pricer PRIVATE ${CMAKE_SOURCE_DIR})
    message(STATUS "pybind11 trouvé — cible cds_pricer activée")
else()
    message(STATUS "pybind11 non trouvé — binding Python ignoré")
endif()
=======
cmake_minimum_required(VERSION 3.16)
project(CDS CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(CDS
        main.cpp

        # Core
        Core/Dates.cpp
        Core/Dates.h
        Core/numericals.h
        Core/types.h

        # Market
        Market/Instruments/CDS/instruments.cpp
        Market/Instruments/CDS/instruments.h
        Market/Curves/YieldCurve/YieldCurve.cpp
        Market/Curves/YieldCurve/YieldCurve.h
        Market/Curves/CDS/CreditCurve.cpp
        Market/Curves/CDS/CreditCurve.h
        Market/Curves/Tranches/BaseCorrelationCurve.cpp
        Market/Curves/Tranches/BaseCorrelationCurve.h
        Market/Instruments/Tranches/tranches_instruments.h

        # Pricers
        Pricers/CDS/cds_pricers.cpp
        Pricers/CDS/cds_pricers.h
        Pricers/Tranches/Base/base_tranche_pricer.cpp
        Pricers/Tranches/Base/base_tranche_pricer.h
        Pricers/Tranches/Methods/lhp_pricer.cpp
        Pricers/Tranches/Methods/lhp_pricer.h
        Pricers/Tranches/Methods/full_recursion_pricing.cpp
        Pricers/Tranches/Methods/full_recursion_pricing.h
        Pricers/Tranches/Methods/gaussian_pricer.cpp
        Pricers/Tranches/Methods/gaussian_pricer.h
        Pricers/Tranches/Methods/binomial_pricer.cpp
        Pricers/Tranches/Methods/binomial_pricer.h
        Pricers/tranche_book_pricer.cpp
        Pricers/tranche_book_pricer.h
        Market/Calibration/base_correlation_curve.h

        # Risk
        Risk/Tranches/risk_engine.h
        Risk/Tranches/pricers_traits.h
        Risk/Tranches/risk_result.h
        Risk/Tranches/bumper.h
        Risk/CDS/risk_engine.cpp
        Risk/CDS/risk_engine.h
        Risk/CDS/bumper.h
)

# PROJECT_SOURCE_DIR = dossier contenant ce CMakeLists.txt
# Tous les includes style "Core/Dates.h" seront résolus depuis là
target_include_directories(CDS PRIVATE ${PROJECT_SOURCE_DIR})
>>>>>>> 065463a (Massive refactor: project restructuring, file organization, code formatting)
