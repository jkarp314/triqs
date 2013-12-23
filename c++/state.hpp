#pragma once

#include <triqs/arrays.hpp>

#include <ostream>
#include <unordered_map>
#include <cmath>
#include <boost/operators.hpp>

#include "hilbert_space.hpp"

namespace cthyb_krylov {

inline double conj(double x) { return x; }

// States of a Hilbert space : can either be described by a map
// or by a triqs::vector so there are two implementations controlled by BasedOnMap
template <typename HilbertSpace, typename ScalarType, bool BasedOnMap> class state {};

template <typename HilbertSpace, typename ScalarType, bool BasedOnMap> 
state<HilbertSpace, ScalarType, BasedOnMap> make_zero_state(state<HilbertSpace, ScalarType, BasedOnMap> const& st) {
 return {st.get_hilbert()};
}

// -----------------------------------------------------------------------------------
// implementation based on a map : can work
// on huge hilbert spaces as long as there are not too
// many components in the state and not too many monomials
//  in the operator acting on the state...
// -----------------------------------------------------------------------------------
template <typename HilbertSpace, typename ScalarType>
class state<HilbertSpace, ScalarType, true> : boost::additive<state<HilbertSpace, ScalarType, true>>,
                                              boost::multiplicative<state<HilbertSpace, ScalarType, true>, ScalarType> {
 // derivations implement the vector space operations over ScalarType from the compounds operators +=, *=, ....
 const HilbertSpace* hs;
 using amplitude_t = std::unordered_map<std::size_t, ScalarType>;
 amplitude_t ampli;

 public:
 using scalar_t = ScalarType;
 using value_type = ScalarType; // only for use with the Krylov (concept to write).

 state() : hs(nullptr) {} // non valid state !
 state(HilbertSpace const& hs_) : hs(&hs_) {}

 scalar_t& operator()(int i) { return ampli[i]; }
 amplitude_t const& amplitudes() const { return ampli; }
 amplitude_t& amplitudes() { return ampli; }
 HilbertSpace const& get_hilbert() const { return *hs; }
 void set_hilbert(HilbertSpace const& hs_) { hs = &hs_; }

 // basic operations
 state& operator+=(state const& s2) {
  for (auto const& aa : s2.ampli) {
   auto r = ampli.insert(aa);
   if (!r.second) r.first->second += aa.second;
  }
  prune();
  return *this;
 }

 state& operator-=(state const& s2) {
  for (auto const& aa : s2.ampli) {
   auto r = ampli.insert({aa.first, -aa.second});
   if (!r.second) r.first->second -= aa.second;
  }
  prune();
  return *this;
 }

 state& operator*=(scalar_t x) {
  for (auto& a : ampli) {
   a.second *= x;
  }
  prune();
  return *this;
 }

 state& operator/=(scalar_t x) {
  (*this) *= 1 / x;
  return *this;
 }

 friend std::ostream& operator<<(std::ostream& os, state const& s) {
  for (auto const& a : s.ampli) {
   os << " +(" << a.second << ")"
      << "|" << s.hs->get_fock_state(a.first) << ">";
  }
  return os;
 }

 // scalar product
 friend scalar_t dot_product(state const& s1, state const& s2) {
  scalar_t res = 0.0;
  for (auto const& a : s1.ampli) {
   if (s2.ampli.count(a.first) == 1) res += conj(a.second) * s2.ampli.at(a.first);
  }
  return res;
 }

 friend bool is_zero_state(state const& st) { return st.amplitudes().size() == 0; }

 private:

 void prune(double tolerance = 10e-10) {
  for (auto it = ampli.begin(); it != ampli.end(); it++) {
   if (std::fabs(it->second) < tolerance) ampli.erase(it);
  }
 }

};

// Lambda (fs, amplitude)
template <typename HilbertSpace, typename ScalarType, typename Lambda>
void foreach(state<HilbertSpace, ScalarType, true> const& st, Lambda l) {
 for (auto const& p : st.amplitudes()) l(st.get_hilbert().get_fock_state(p.first), p.second);
}

// -----------------------------------------------------------------------------------
// implementation based on a vector
// -----------------------------------------------------------------------------------
template <typename HilbertSpace, typename ScalarType>
class state<HilbertSpace, ScalarType, false> : boost::additive<state<HilbertSpace, ScalarType, false>>,
                                               boost::multiplicative<state<HilbertSpace, ScalarType, false>, ScalarType> {

 const HilbertSpace* hs;
 using amplitude_t = triqs::arrays::vector<ScalarType>;
 amplitude_t ampli;

 public:
 using scalar_t = ScalarType;
 using value_type = ScalarType; // only for use with the Krylov (concept to write).

 state() : hs(nullptr) {}
 state(HilbertSpace const& hs_) : hs(&hs_), ampli(hs_.dimension(), 0.0) {}

 // full access to amplitudes
 amplitude_t const& amplitudes() const { return ampli; }
 amplitude_t& amplitudes() { return ampli; }

 // access to data
 scalar_t& operator()(int i) { return ampli[i]; }
 scalar_t const& operator()(int i) const { return ampli[i]; }

 // get access to hilbert space
 HilbertSpace const& get_hilbert() const { return *hs; }
 void set_hilbert(HilbertSpace const& hs_) { hs = &hs_; }

 // print
 friend std::ostream& operator<<(std::ostream& os, state const& s) {
  bool something_written = false;
  for (int i = 0; i < s.ampli.size(); ++i) {
   auto ampl = s(i);
   if (std::abs(ampl) < 1e-10) continue;
   os << " +(" << ampl << ")"
      << "|" << s.hs->get_fock_state(i) << ">";
   something_written = true;
  }
  if (!something_written) os << 0;
  return os;
 }

 // basic operations
 state& operator+=(state const& s2) {
  ampli += s2.ampli;
  return *this;
 }

 state& operator-=(state const& s2) {
  ampli -= s2.ampli;
  return *this;
 }

 state& operator*=(scalar_t x) {
  ampli *= x;
  return *this;
 }

 state& operator/=(scalar_t x) {
  ampli /= x;
  return *this;
 }

 // scalar product
 friend scalar_t dot_product(state const& s1, state const& s2) { return dotc(s1.ampli, s2.ampli); }

 // TO BE REMOVED : tolerance !!
 friend bool is_zero_state(state const& st, double tolerance = 1e-18) {
  if (st.amplitudes().size() == 0) return true;
  //for (auto const& a : st.amplitudes())
  // if (std::fabs(a) > tolerance) return false;
  return false;
  return true;
 }
};

// Lambda (fs, amplitude)
template <typename HilbertSpace, typename ScalarType, typename Lambda>
void foreach(state<HilbertSpace, ScalarType, false> const& st, Lambda l) {
 const auto L = st.amplitudes().size();
 for (size_t i = 0; i < L; ++i) l(st.get_hilbert().get_fock_state(i), st.amplitudes()[i]);
}
}