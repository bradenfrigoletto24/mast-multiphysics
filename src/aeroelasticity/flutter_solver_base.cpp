/*
 * MAST: Multidisciplinary-design Adaptation and Sensitivity Toolkit
 * Copyright (C) 2013-2016  Manav Bhatia
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// MAST includes
#include "aeroelasticity/flutter_solver_base.h"
#include "aeroelasticity/flutter_solution_base.h"
#include "aeroelasticity/flutter_root_base.h"
#include "aeroelasticity/flutter_root_crossover_base.h"
#include "elasticity/structural_fluid_interaction_assembly.h"
#include "elasticity/piston_theory_boundary_condition.h"
#include "base/physics_discipline_base.h"
#include "base/boundary_condition_base.h"
#include "numerics/lapack_dggev_interface.h"
#include "base/parameter.h"


MAST::FlutterSolverBase::FlutterSolverBase():
_assembly(NULL),
_basis_vectors(NULL),
_output(NULL),
_steady_solver(NULL) {
    
}



MAST::FlutterSolverBase::~FlutterSolverBase() {
    
    _assembly         = NULL;
    _basis_vectors    = NULL;
    if (_output)
        delete _output;
}




void
MAST::FlutterSolverBase::
attach_assembly(MAST::StructuralFluidInteractionAssembly&   assembly) {
    
    // make sure that the assembly is not already set
    libmesh_assert(!_assembly);
    
    _assembly = &assembly;
}




void
MAST::FlutterSolverBase::clear() {
    
    _assembly         = NULL;
    _basis_vectors    = NULL;
    if (_output) {
        delete _output;
        _output = NULL;
    }
}



void
MAST::FlutterSolverBase::attach_steady_solver(MAST::FlutterSolverBase::SteadySolver &solver) {
    
    _steady_solver = &solver;
}



void
MAST::FlutterSolverBase::clear_assembly_object() {
    
    _assembly      = NULL;
    _steady_solver = NULL;
}




void
MAST::FlutterSolverBase::
initialize(std::vector<libMesh::NumericVector<Real> *>& basis) {
    
    
    _basis_vectors  = &basis;
}




