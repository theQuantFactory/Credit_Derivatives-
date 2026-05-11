//
// Created by ricardo on 25‏/3‏/2026.
//

#pragma once
#include <algorithm>
#include <stdexcept>

namespace Core {

    class NewtonRaphson {
        public:

        NewtonRaphson() = delete;
        NewtonRaphson(const double tol, const int iter_max) : m_tol(tol), m_iter_max(iter_max) {
        }

        template<typename F>
        double solve(F&& f_and_df, const double x0) const {
            double x = x0;
            for (int i = 0; i < m_iter_max; ++i) {
                auto [f, df] = f_and_df(x);
                if (std::abs(f) < m_tol) break;
                x -= f / df;
                x  = std::max(x, 1e-6);
            }
            return x;
        }

        private:
        double m_tol;
        int m_iter_max;
    };

    class Brent {
    public :
        Brent() = delete;
        Brent(const double tol, const int iter_max) : m_tol(tol), m_iter_max(iter_max) {
        }

        template<typename F>

         double solve(F&& f, double a, double b) {

                double fa = f(a), fb = f(b);
                if (fa * fb > 0.0)
                    throw std::runtime_error("Brent : f(a) and f(b) got the same sign ");

                double c = a, fc = fa, d = b-a, e = d;
                for (int i = 0; i < m_iter_max; ++i) {
                    if (fb * fc > 0.0) { c = a; fc = fa; d = e = b-a; }
                    if (std::abs(fc) < std::abs(fb)) {
                        a = b; fa = fb; b = c; fb = fc; c = a; fc = fa;
                    }
                    const double tol1 = 2.0*m_tol*std::abs(b) + 0.5*m_tol;
                    const double m    = 0.5*(c - b);
                    if (std::abs(m) <= tol1 || fb == 0.0) return b;

                    if (std::abs(e) >= tol1 && std::abs(fa) > std::abs(fb)) {
                        double s = fb/fa, p, q, r;
                        if (a == c) {
                            p = 2.0*m*s; q = 1.0-s;
                        } else {
                            q = fa/fc; r = fb/fc;
                            p = s*(2.0*m*q*(q-r) - (b-a)*(r-1.0));
                            q = (q-1.0)*(r-1.0)*(s-1.0);
                        }
                        if (p > 0.0) q = -q; else p = -p;
                        if (2.0*p < std::min(3.0*m*q - std::abs(tol1*q), std::abs(e*q))) {
                            e = d; d = p/q;
                        } else { d = m; e = d; }
                    } else { d = m; e = d; }

                    a = b; fa = fb;
                    b += (std::abs(d) > tol1) ? d : (m > 0 ? tol1 : -tol1);
                    fb = f(b);
                }
                throw std::runtime_error("Brent : No convergence of the algorithm");
            }


    private:
        double m_tol;
        int m_iter_max;
    };









}
