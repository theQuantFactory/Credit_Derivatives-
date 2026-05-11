<<<<<<< HEAD
 //
// Created by ricardo on 25‏/3‏/2026.
//

#include "Dates.hpp"
#include <algorithm>
=======

#include "Dates.h"
#include <chrono>
>>>>>>> 065463a (Massive refactor: project restructuring, file organization, code formatting)
#include <cmath>

namespace Core {

<<<<<<< HEAD
  Date::Date(const int year, const int month, const int day) {
    m_julian_days = toJulian(year, month, day);
  };

  Date::Date(const long long julian_day) : m_julian_days(julian_day) {}

  Date::Date() {
    const auto today = std::chrono::system_clock::now();
    const auto days  = std::chrono::floor<std::chrono::days>(today);
=======
long long Date::toJDN(const int year, const int month, const int day)
{
    const int a = (14 - month) / 12;
    const int y = year + 4800 - a;
    const int m = month + 12 * a - 3;
    return day
        + (153 * m + 2) / 5
        + 365LL * y
        + y / 4
        - y / 100
        + y / 400
        - 32045;
}

std::tuple<int,int,int> Date::fromJDN(const long long jdn)
{
    const long long a = jdn + 32044;
    const long long b = (4 * a + 3) / 146097;
    const long long c = a - (146097 * b) / 4;
    const long long d = (4 * c + 3) / 1461;
    const long long e = c - (1461 * d) / 4;
    const long long m = (5 * e + 2) / 153;

    const int day   = static_cast<int>(e - (153 * m + 2) / 5 + 1);
    const int month = static_cast<int>(m + 3 - 12 * (m / 10));
    const int year  = static_cast<int>(100 * b + d - 4800 + m / 10);

    return { year, month, day };
}

bool Date::isLeapYear(const int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int Date::daysInMonth(const int month, const int year)
{
    static constexpr int days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}


Date::Date(const int year, const int month, const int day)
    : m_jdn(toJDN(year, month, day))
{}

Date::Date(const long long jdn)
    : m_jdn(jdn)
{}

Date::Date()
{
    const auto now  = std::chrono::system_clock::now();
    const auto days = std::chrono::floor<std::chrono::days>(now);
>>>>>>> 065463a (Massive refactor: project restructuring, file organization, code formatting)
    const std::chrono::year_month_day ymd(days);

    const int y = static_cast<int>(ymd.year());
    const int m = static_cast<int>(static_cast<unsigned>(ymd.month()));
    const int d = static_cast<int>(static_cast<unsigned>(ymd.day()));

<<<<<<< HEAD
    m_julian_days = toJulian(y, m, d);
  }

  long long Date::getJulianDays() const { return m_julian_days; }

  Date Date::add_days(const int days) const {
    return Date(m_julian_days + days);
  }

  Date Date::add_months(const int n_months) const {
    if (n_months == 0) return *this;
    auto [y,m,d] = fromJulian(m_julian_days);
    long long total_days = 0;
    total_days += days_in_month(m,y) - d;
    m++;
    if (m > 12) { m = 1; y++; }
    for (int i = 1; i < n_months; ++i) {
      total_days += days_in_month(m, y);
      m++;
      if (m > 12) { m = 1; y++; }
    }
    const int dim = days_in_month(m, y);
    total_days += std::min(d, dim);
    return Date(m_julian_days + total_days);
  }

  Date Date::add_months(const double n_month) const {
    const int whole_months = static_cast<int>(n_month);
    const double fraction = n_month - whole_months;

    Date result = add_months(whole_months);

    if (fraction > 0.0) {
      auto [year, month, day] = fromJulian(result.m_julian_days);
      const int dim = days_in_month(month, year);
      const int days_to_add = static_cast<int>(std::round(fraction * dim));
      result = result.add_days(days_to_add);
    }

    return result;

  }

  bool Date::is_leap_year(const int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  }

  int Date::days_in_month(const int month, const int year) {
    if (month == 2 && is_leap_year(year)) {
      return 29;
    }
    return days_month[month-1];
  }

  long long Date::toJulian(const int year, const int month, const int day) // Function to convert to Julian
  {
    const long long jd = day - 32075
            + 1461 * (year + 4800 + (month - 14) / 12) / 4
            + 367 * (month - 2 - ((month - 14) / 12) * 12) / 12
            - 3 * ((year + 4900 + (month - 14) / 12) / 100) / 4 ;

    return jd;
  }

  std::tuple<int,int,int> Date::fromJulian(const long long julian_day) { // Function to convert to gregorian date
    long l = julian_day + 68569;
    const long n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    const long i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    const long j = 80 * l / 2447;
    int day = static_cast<int>(l - 2447 * j / 80);
    l = j / 11;
    int month = static_cast<int>(j + 2 - 12 * l);
    int year = static_cast<int> (100 * (n - 49) + i + l);

    return std::make_tuple(year, month, day);
  }

}
=======
    m_jdn = toJDN(y, m, d);
}


long long Date::getJulianDays() const { return m_jdn; }

std::tuple<int,int,int> Date::toGregorian() const { return fromJDN(m_jdn); }


Date Date::add_days(const int days) const
{
    return Date(m_jdn + days);
}

Date Date::add_months(const int n_months) const
{
    if (n_months == 0) return *this;

    auto [y, m, d] = fromJDN(m_jdn);

    const int total  = (y * 12 + (m - 1)) + n_months;
    int new_year     =  total / 12;
    int new_month    = (total % 12) + 1;

    if (new_month < 1) {
        new_month += 12;
        new_year  -= 1;
    }

    const int dim     = daysInMonth(new_month, new_year);
    const int new_day = (d > dim) ? dim : d;

    return {new_year, new_month, new_day};
}

Date Date::add_months(const double n_months) const
{
    const int    whole    = static_cast<int>(std::trunc(n_months));
    const double fraction = n_months - whole;

    Date result = add_months(whole);

    if (std::abs(fraction) > 1e-9) {
        auto [y, m, d] = fromJDN(result.m_jdn);
        const int dim  = daysInMonth(m, y);
        const int extra = static_cast<int>(std::round(fraction * dim));
        result = result.add_days(extra);
    }

    return result;
}

}
>>>>>>> 065463a (Massive refactor: project restructuring, file organization, code formatting)
