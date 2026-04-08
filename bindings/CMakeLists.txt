cmake_minimum_required(VERSION 3.15)
project(cds_pricer_bindings)

set(CMAKE_CXX_STANDARD 20)

# ── Locate Python & pybind11 ──────────────────────────────────────────────────
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

# ── Path to your C++ sources ──────────────────────────────────────────────────
# Adjust SRC_DIR to point to the root of "CDS and IndexTranches Pricing"
# By default we assume the C++ project is one level up, in a folder called CDS_src.
# Override at configure time:  cmake .. -DSRC_DIR=/absolute/path/to/your/sources
if(NOT DEFINED SRC_DIR)
    set(SRC_DIR "${CMAKE_SOURCE_DIR}/../CDS_src")
endif()

message(STATUS "C++ source root : ${SRC_DIR}")

set(LIB_SOURCES
    "${SRC_DIR}/Core/Dates.cpp"
    "${SRC_DIR}/Core/interpolator.cpp"
    "${SRC_DIR}/Market/instruments.cpp"
    "${SRC_DIR}/Market/YieldCurve.cpp"
    "${SRC_DIR}/Market/CreditCurve.cpp"
    "${SRC_DIR}/Pricers/pricers.cpp"
)

# ── Build the Python extension module ────────────────────────────────────────
pybind11_add_module(cds_pricer
    ${CMAKE_SOURCE_DIR}/bindings.cpp
    ${LIB_SOURCES}
)

target_include_directories(cds_pricer PRIVATE "${SRC_DIR}")
