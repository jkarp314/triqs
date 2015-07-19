#!/bin/env pytriqs

import pytriqs.utility.mpi as mpi
from pytriqs.gf.local import *
from pytriqs.operators import *
from pytriqs.archive import HDFArchive
from pytriqs.applications.impurity_solvers.cthyb import *

mpi.report("Welcome to asymm_bath test (1 band with a small asymmetric hybridization function).")
mpi.report("This test helps to detect sampling problems.")

# H_loc parameters
beta = 40.0
ed = -1.0
U = 2.0

epsilon = [0.0,0.1,0.2,0.3,0.4,0.5]
V = 0.2

# Parameters
n_iw = 1025
n_tau = 10001

p = {}
p["max_time"] = -1
p["random_name"] = ""
p["random_seed"] = 123 * mpi.rank + 567
p["length_cycle"] = 50
p["n_warmup_cycles"] = 20000
p["n_cycles"] = 1000000
p["performance_analysis"] = True
p["measure_pert_order"] = True

# Block structure of GF
gf_struct = {'up':[0], 'dn':[0]}

# Hamiltonian
H = U*n("up",0)*n("dn",0)

# Histograms to be saved
histos_to_save = [('histo_pert_order_up',int),
                  ('histo_pert_order_dn',int),
                  ('histo_pert_order',int),
                  ('histo_insert_length_proposed',float),
                  ('histo_insert_length_accepted',float),
                  ('histo_remove_length_proposed',float),
                  ('histo_remove_length_accepted',float),
                  ('histo_shift_length_proposed',float),
                  ('histo_shift_length_accepted',float)]

# Quantum numbers
qn = [n("up",0),n("dn",0)]
p["partition_method"] = "quantum_numbers"
p["quantum_numbers"] = qn

# Construct solver
S = SolverCore(beta=beta, gf_struct=gf_struct, n_tau=n_tau, n_iw=n_iw)

def read_histo(f,type_of_col_1):
    histo = []
    for line in f:
        cols = filter(lambda s: s, line.split(' '))
        histo.append((type_of_col_1(cols[0]),float(cols[1]),float(cols[2])))
    return histo

if mpi.is_master_node():
    arch = HDFArchive('asymm_bath.h5','w')

# Set hybridization function
for e in epsilon:
    delta_w = GfImFreq(indices = [0], beta=beta)
    delta_w << (V**2) * inverse(iOmega_n - e)

    S.G0_iw["up"] << inverse(iOmega_n - ed - delta_w)
    S.G0_iw["dn"] << inverse(iOmega_n - ed - delta_w)

    S.solve(h_int=H, **p)

    if mpi.is_master_node():
        d = {'G_tau':S.G_tau, 'beta':beta, 'U':U, 'ed':ed, 'V':V, 'e':e}

        for histo_name, type_of_col_1 in histos_to_save:
            d[histo_name] = read_histo(open(histo_name+'.dat','r'),type_of_col_1)

        arch['epsilon_' + str(e)] = d