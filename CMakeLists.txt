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