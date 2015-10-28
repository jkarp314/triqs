/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2014, P. Seth, I. Krivenko, M. Ferrero and O. Parcollet
 *
 * TRIQS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * TRIQS. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#pragma once
#include <triqs/mc_tools.hpp>
#include <triqs/utility/callbacks.hpp>
#include <triqs/operators/many_body_operator.hpp>
#include "solve_parameters.hpp"
#include "./qmc_data.hpp"

namespace cthyb {

using namespace triqs::utility;
using mc_weight_type = double;
using mc_sign_type = mc_weight_type;
using indices_type = triqs::operators::indices_t;
using many_body_op_type = triqs::operators::many_body_operator;

using h_loc_diagonalization = sorted_spaces;

class solver_core {

 double beta;
 h_loc_diagonalization hdiag;
 std::map<std::string, indices_type> gf_struct;
 many_body_op_type _h_loc;                  // The local Hamiltonian = h_int + h0
 block_gf<imfreq> _G0_iw;                   // Green's function containers: imaginary-freq Green's functions
 block_gf<imtime> _Delta_tau, _G_tau;       // Green's function containers: imaginary-time Green's functions
 block_gf<imtime,matrix_real_valued> _G_tau_real;
 block_gf<legendre> _G_l;                   // Green's function containers: Legendre coefficients
 std::vector<matrix<double>> _density_matrix; // density matrix, when used in Norm mode
 triqs::mpi::communicator _comm;            // define the communicator, here MPI_COMM_WORLD
 solve_parameters_t _last_solve_parameters; // parameters of the last call to solve
 mc_sign_type _average_sign;
 int _solve_status;                         // Status of the solve upon exit: 0 for clean termination, > 0 otherwise.

 public:

 solver_core(double beta, std::map<std::string, indices_type> const & gf_struct, int n_iw=1025, int n_tau=10001, int n_l=50);

 /// Solve the impurity problem for the given Hamiltonian h_loc and with specified parameters params.
 TRIQS_WRAP_ARG_AS_DICT // Wrap the solver parameters as a dictionary in python with the clang tool
 void solve(solve_parameters_t const & p);

 /// The local Hamiltonian of the problem : H_loc used in the last call to solve.
 many_body_op_type const & h_loc() const { return _h_loc; }

 /// Set of parameters used in the last call to solve
 solve_parameters_t get_last_solve_parameters() const {return _last_solve_parameters;}

 /// G0(iw) in imaginary frequencies
 block_gf_view<imfreq> G0_iw() { return _G0_iw; }

 /// Delta(tau) in imaginary time
 block_gf_view<imtime> Delta_tau() { return _Delta_tau; }

 /// G(tau) in imaginary time
 block_gf_view<imtime> G_tau() { return _G_tau; }
 
 /// G_l in Legendre polynomials representation
 block_gf_view<legendre> G_l() { return _G_l; }

 /// Atomic G(tau) in imaginary time
 block_gf_view<imtime> atomic_gf() const { return hdiag.atomic_gf(beta,gf_struct,_G_tau[0].mesh().size()); }

 /// Density matrix
 std::vector<matrix<double>> const & density_matrix() const { return _density_matrix;}

 /// Access to the Hloc diagonalization
 h_loc_diagonalization const & get_h_loc_diagonalization() const { return hdiag;}

 /// Monte Carlo average sign
 mc_sign_type average_sign() const { return _average_sign; }

 /// Status of the solve on exit
 int solve_status() const { return _solve_status; }

};

}