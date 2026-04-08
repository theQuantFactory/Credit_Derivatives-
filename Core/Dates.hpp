//
// Created by ricardo on 25‏/3‏/2026.
//

#pragma once
#include <tuple>
#include <chrono>


namespace Core {

    class Date {
    public:

        Date(int year, int month, int day);
        explicit Date(long long julian_day);
        Date();

        [[nodiscard]] long long getJulianDays() const;

        [[nodiscard]] Date add_days(int days) const;
        [[nodiscard]] Date add_months(int n_months) const;
        [[nodiscard]] Date add_months(double n_month) const;
        static std::tuple<int,int,int> fromJulian(long long julian_day);

        long long operator - (const Date& other) const {return m_julian_days - other.m_julian_days;};
        bool operator<(const Date& other) const {
            return m_julian_days < other.m_julian_days;
        }

        bool operator>(const Date& other) const {
            return m_julian_days > other.m_julian_days;
        }

        bool operator==(const Date& other) const {
            return m_julian_days == other.m_julian_days;
        }

        bool operator!=(const Date& other) const {
            return m_julian_days != other.m_julian_days;
        }

        bool operator<=(const Date& other) const {
            return m_julian_days <= other.m_julian_days;
        }

        bool operator>=(const Date& other) const {
            return m_julian_days >= other.m_julian_days;
        }

    private:
        long long m_julian_days ;
        static constexpr int days_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        static long long toJulian(int year, int month, int day);

        static bool is_leap_year(int year) ;
        static int days_in_month(int month, int year);

    };







}