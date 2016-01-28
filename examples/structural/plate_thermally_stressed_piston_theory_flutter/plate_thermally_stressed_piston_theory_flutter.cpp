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

// C++ includes
#include <iostream>


// MAST includes
#include "examples/structural/plate_thermally_stressed_piston_theory_flutter/plate_thermally_stressed_piston_theory_flutter.h"
#include "examples/base/multilinear_interpolation.h"
#include "base/nonlinear_system.h"
#include "base/parameter.h"
#include "base/constant_field_function.h"
#include "elasticity/structural_system_initialization.h"
#include "elasticity/structural_discipline.h"
#include "elasticity/structural_element_base.h"
#include "elasticity/structural_modal_eigenproblem_assembly.h"
#include "elasticity/structural_fluid_interaction_assembly.h"
#include "elasticity/piston_theory_boundary_condition.h"
#include "elasticity/stress_output_base.h"
#include "elasticity/structural_nonlinear_assembly.h"
#include "aeroelasticity/time_domain_flutter_solver.h"
#include "aeroelasticity/time_domain_flutter_root_base.h"
#include "property_cards/solid_2d_section_element_property_card.h"
#include "property_cards/isotropic_material_property_card.h"
#include "boundary_condition/dirichlet_boundary_condition.h"
#include "driver/driver_base.h"

// libMesh includes
#include "libmesh/mesh_generation.h"
#include "libmesh/exodusII_io.h"
#include "libmesh/numeric_vector.h"


extern libMesh::LibMeshInit* __init;


MAST::PlateThermallyStressedPistonTheoryFlutterAnalysis::
PlateThermallyStressedPistonTheoryFlutterAnalysis():
_initialized(false) {
    
}



void
MAST::PlateThermallyStressedPistonTheoryFlutterAnalysis::
init(libMesh::ElemType e_type, bool if_vk) {
    
    
    libmesh_assert(!_initialized);
    
    
    // length of domain
    _length     = 0.30,
    _width      = 0.30;
    
    
    // create the mesh
    _mesh       = new libMesh::SerialMesh(__init->comm());
    
    // initialize the mesh with one element
    libMesh::MeshTools::Generation::build_square(*_mesh,
                                                 32, 32,
                                                 0, _length,
                                                 0, _width,
                                                 e_type);
    _mesh->prepare_for_use();
    
    // create the equation system
    _eq_sys    = new  libMesh::EquationSystems(*_mesh);
    
    // create the libmesh system
    _sys       = &(_eq_sys->add_system<MAST::NonlinearSystem>("structural"));
    _sys->set_eigenproblem_type(libMesh::GHEP);
    
    
    // FEType to initialize the system
    libMesh::FEType fetype (libMesh::FIRST, libMesh::LAGRANGE);
    
    // initialize the system to the right set of variables
    _structural_sys = new MAST::StructuralSystemInitialization(*_sys,
                                                               _sys->name(),
                                                               fetype);
    _discipline     = new MAST::StructuralDiscipline(*_eq_sys);
    
    
    // create and add the boundary condition and loads
    _dirichlet_bottom = new MAST::DirichletBoundaryCondition;
    _dirichlet_right  = new MAST::DirichletBoundaryCondition;
    _dirichlet_top    = new MAST::DirichletBoundaryCondition;
    _dirichlet_left   = new MAST::DirichletBoundaryCondition;
    
    std::vector<unsigned int> constrained_vars(3);
    constrained_vars[0] = 0;  // u
    constrained_vars[1] = 1;  // v
    constrained_vars[2] = 2;  // w
    
    _dirichlet_bottom->init (0, constrained_vars);
    _dirichlet_right->init  (1, constrained_vars);
    _dirichlet_top->init    (2, constrained_vars);
    _dirichlet_left->init   (3, constrained_vars);
    
    _discipline->add_dirichlet_bc(0, *_dirichlet_bottom);
    _discipline->add_dirichlet_bc(1,  *_dirichlet_right);
    _discipline->add_dirichlet_bc(2,    *_dirichlet_top);
    _discipline->add_dirichlet_bc(3,   *_dirichlet_left);
    _discipline->init_system_dirichlet_bc(*_sys);
    
    // initialize the equation system
    _eq_sys->init();
    
    _sys->eigen_solver->set_position_of_spectrum(libMesh::LARGEST_MAGNITUDE);
    _sys->set_exchange_A_and_B(true);
    _sys->set_n_requested_eigenvalues(10);
    
    
    // create the property functions and add them to the
    
    _th              = new MAST::Parameter("th",          0.006);
    _E               = new MAST::Parameter("E",           72.e9);
    _rho             = new MAST::Parameter("rho",         2.8e3);
    _nu              = new MAST::Parameter("nu",           0.33);
    _alpha           = new MAST::Parameter("alpha",      2.5e-5);
    _kappa           = new MAST::Parameter("kappa",       5./6.);
    _zero            = new MAST::Parameter("zero",           0.);
    _velocity        = new MAST::Parameter("V"   ,        8000.);
    _mach            = new MAST::Parameter("mach",           3.);
    _rho_air         = new MAST::Parameter("rho" ,         1.05);
    _gamma_air       = new MAST::Parameter("gamma",         1.4);
    _temp            = new MAST::Parameter("temperature",   90.);
    
    
    
    // prepare the vector of parameters with respect to which the sensitivity
    // needs to be benchmarked
    _params_for_sensitivity.push_back( _E);
    _params_for_sensitivity.push_back(_nu);
    _params_for_sensitivity.push_back(_th);
    
    
    
    _th_f            = new MAST::ConstantFieldFunction("h",           *_th);
    _E_f             = new MAST::ConstantFieldFunction("E",            *_E);
    _nu_f            = new MAST::ConstantFieldFunction("nu",          *_nu);
    _rho_f           = new MAST::ConstantFieldFunction("rho",        *_rho);
    _alpha_f         = new MAST::ConstantFieldFunction("alpha_expansion", *_alpha);
    _kappa_f         = new MAST::ConstantFieldFunction("kappa",    *_kappa);
    _velocity_f      = new MAST::ConstantFieldFunction("V",      *_velocity);
    _mach_f          = new MAST::ConstantFieldFunction("mach",       *_mach);
    _rho_air_f       = new MAST::ConstantFieldFunction("rho",     *_rho_air);
    _gamma_air_f     = new MAST::ConstantFieldFunction("gamma", *_gamma_air);
    _temp_f          = new MAST::ConstantFieldFunction("temperature", *_temp);
    _ref_temp_f      = new MAST::ConstantFieldFunction("ref_temperature", *_zero);
    _hoff_f          = new MAST::SectionOffset("off",
                                               _th_f->clone().release(),
                                               1.);

    
    // initialize the load
    _T_load          = new MAST::BoundaryConditionBase(MAST::TEMPERATURE);
    _T_load->add(*_temp_f);
    _T_load->add(*_ref_temp_f);
    _discipline->add_volume_load(0, *_T_load);

    // create the material property card
    _m_card         = new MAST::IsotropicMaterialPropertyCard;
    
    // add the material properties to the card
    _m_card->add(*_E_f);
    _m_card->add(*_nu_f);
    _m_card->add(*_kappa_f);
    _m_card->add(*_alpha_f);
    _m_card->add(*_rho_f);
    
    
    // create the element property card
    _p_card         = new MAST::Solid2DSectionElementPropertyCard;
    
    // add the section properties to the card
    _p_card->add(*_th_f);
    _p_card->add(*_hoff_f);
    
    // tell the section property about the material property
    _p_card->set_material(*_m_card);
    if (if_vk) _p_card->set_strain(MAST::VON_KARMAN_STRAIN);
    
    _discipline->set_property_for_subdomain(0, *_p_card);

    
    // create the output objects, one for each element
    libMesh::MeshBase::const_element_iterator
    e_it    = _mesh->elements_begin(),
    e_end   = _mesh->elements_end();
    
    // points where stress is evaluated
    std::vector<libMesh::Point> pts;
    
    if (e_type == libMesh::QUAD4 ||
        e_type == libMesh::QUAD8 ||
        e_type == libMesh::QUAD9) {
        
        pts.push_back(libMesh::Point(-1/sqrt(3), -1/sqrt(3), 1.)); // upper skin
        pts.push_back(libMesh::Point(-1/sqrt(3), -1/sqrt(3),-1.)); // lower skin
        pts.push_back(libMesh::Point( 1/sqrt(3), -1/sqrt(3), 1.)); // upper skin
        pts.push_back(libMesh::Point( 1/sqrt(3), -1/sqrt(3),-1.)); // lower skin
        pts.push_back(libMesh::Point( 1/sqrt(3),  1/sqrt(3), 1.)); // upper skin
        pts.push_back(libMesh::Point( 1/sqrt(3),  1/sqrt(3),-1.)); // lower skin
        pts.push_back(libMesh::Point(-1/sqrt(3),  1/sqrt(3), 1.)); // upper skin
        pts.push_back(libMesh::Point(-1/sqrt(3),  1/sqrt(3),-1.)); // lower skin
    }
    else if (e_type == libMesh::TRI3 ||
             e_type == libMesh::TRI6) {
        
        pts.push_back(libMesh::Point(1./3., 1./3., 1.)); // upper skin
        pts.push_back(libMesh::Point(1./3., 1./3.,-1.)); // lower skin
        pts.push_back(libMesh::Point(2./3., 1./3., 1.)); // upper skin
        pts.push_back(libMesh::Point(2./3., 1./3.,-1.)); // lower skin
        pts.push_back(libMesh::Point(1./3., 2./3., 1.)); // upper skin
        pts.push_back(libMesh::Point(1./3., 2./3.,-1.)); // lower skin
    }
    else
        libmesh_assert(false); // should not get here
    
    for ( ; e_it != e_end; e_it++) {
        
        MAST::StressStrainOutputBase * output = new MAST::StressStrainOutputBase;
        
        // tell the object to evaluate the data for this object only
        std::set<const libMesh::Elem*> e_set;
        e_set.insert(*e_it);
        output->set_elements_in_domain(e_set);
        output->set_points_for_evaluation(pts);
        _outputs.push_back(output);
        
        _discipline->add_volume_output((*e_it)->subdomain_id(), *output);
    }
    

    
    // now initialize the piston theory boundary conditions
    RealVectorX  vel = RealVectorX::Zero(3);
    vel(0)           = 1.;  // flow along the x-axis
    _piston_bc       = new MAST::PistonTheoryBoundaryCondition(1,     // order
                                                               vel);  // vel vector
    _piston_bc->add(*_velocity_f);
    _piston_bc->add(*_mach_f);
    _piston_bc->add(*_rho_air_f);
    _piston_bc->add(*_gamma_air_f);
    _discipline->add_volume_load(0, *_piston_bc);
    
    
    // initialize the flutter solver
    _flutter_solver  = new MAST::TimeDomainFlutterSolver;
    std::string nm("flutter_output.txt");
    _flutter_solver->set_output_file(nm);

    _initialized = true;
}







MAST::PlateThermallyStressedPistonTheoryFlutterAnalysis::
~PlateThermallyStressedPistonTheoryFlutterAnalysis() {
    
    if (_initialized) {
        
        delete _m_card;
        delete _p_card;
        
        delete _dirichlet_bottom;
        delete _dirichlet_right;
        delete _dirichlet_top;
        delete _dirichlet_left;
        
        delete _th_f;
        delete _E_f;
        delete _nu_f;
        delete _rho_f;
        delete _alpha_f;
        delete _kappa_f;
        delete _hoff_f;
        delete _velocity_f;
        delete _mach_f;
        delete _rho_air_f;
        delete _gamma_air_f;
        delete _temp_f;
        delete _ref_temp_f;

        delete _th;
        delete _E;
        delete _nu;
        delete _rho;
        delete _alpha;
        delete _kappa;
        delete _zero;
        delete _velocity;
        delete _mach;
        delete _rho_air;
        delete _gamma_air;
        delete _temp;
        
        // delete the basis vectors
        if (_basis.size())
            for (unsigned int i=0; i<_basis.size(); i++)
                delete _basis[i];
        
        
        delete _eq_sys;
        delete _mesh;
        
        delete _discipline;
        delete _structural_sys;

        delete _flutter_solver;
        delete _piston_bc;
        
        // iterate over the output quantities and delete them
        std::vector<MAST::StressStrainOutputBase*>::iterator
        it   =   _outputs.begin(),
        end  =   _outputs.end();
        
        for ( ; it != end; it++)
            delete *it;
        
        _outputs.clear();
    
    }
}



MAST::Parameter*
MAST::PlateThermallyStressedPistonTheoryFlutterAnalysis::
get_parameter(const std::string &nm) {
    
    libmesh_assert(_initialized);
    
    MAST::Parameter *rval = NULL;
    
    // look through the vector of parameters to see if the name is available
    std::vector<MAST::Parameter*>::iterator
    it   =  _params_for_sensitivity.begin(),
    end  =  _params_for_sensitivity.end();
    
    bool
    found = false;
    
    for ( ; it != end; it++) {
        
        if (nm == (*it)->name()) {
            rval    = *it;
            found   = true;
        }
    }
    
    // if the param was not found, then print the message
    if (!found) {
        std::cout
        << std::endl
        << "Parameter not found by name: " << nm << std::endl
        << "Valid names are: "
        << std::endl;
        for (it = _params_for_sensitivity.begin(); it != end; it++)
            std::cout << "   " << (*it)->name() << std::endl;
        std::cout << std::endl;
    }
    
    return rval;
}




Real
MAST::PlateThermallyStressedPistonTheoryFlutterAnalysis::
solve(bool if_write_output) {
    
    libmesh_assert(_initialized);
    
    bool if_vk = (_p_card->strain_type() == MAST::VON_KARMAN_STRAIN);

    ///////////////////////////////////////////////////////////////
    // first, solve the quasi-steady problem
    ///////////////////////////////////////////////////////////////
    // set the number of load steps
    unsigned int
    n_steps = 1;
    if (if_vk) n_steps = 25;
    
    Real
    T0      = (*_temp)(),
    V0      = (*_velocity)();

    // create the nonlinear assembly object
    MAST::StructuralNonlinearAssembly   nonlin_assembly;
    
    nonlin_assembly.attach_discipline_and_system(*_discipline, *_structural_sys);
    
    libMesh::NonlinearImplicitSystem&      nonlin_sys   =
    dynamic_cast<libMesh::NonlinearImplicitSystem&>(nonlin_assembly.system());
    
    // zero the solution before solving
    nonlin_sys.solution->zero();
    this->clear_stresss();
    
    // now iterate over the load steps
    for (unsigned int i=0; i<n_steps; i++) {
        std::cout
        << "Load step: " << i << std::endl;
        
        (*_temp)()      =  T0*(i+1.)/(1.*n_steps);
        (*_velocity)()  =  V0*(i+1.)/(1.*n_steps);
        nonlin_sys.solve();
    }
    
    // evaluate the outputs
    nonlin_assembly.calculate_outputs(*(_sys->solution));
    
    nonlin_assembly.clear_discipline_and_system();
    
    if (if_write_output) {
        
        std::cout << "Writing output to : output.exo" << std::endl;
        
        std::set<std::string> sys_to_write;
        sys_to_write.insert(_sys->name());
        
        // write the solution for visualization
        libMesh::ExodusII_IO(*_mesh).write_equation_systems("output.exo",
                                                            *_eq_sys,
                                                            &sys_to_write);
        
        _discipline->plot_stress_strain_data<libMesh::ExodusII_IO>("stress_output.exo");
    }
    
    return 0.;
    
    ///////////////////////////////////////////////////////////////
    // next, solve the modal eigenvalue problem
    ///////////////////////////////////////////////////////////////
    // clear out the data structures of the flutter solver before
    // this solution
    _flutter_root = NULL;
    _flutter_solver->clear();
    
    // set the velocity of piston theory to zero for modal analysis
    (*_velocity) = 0.;

    // create the nonlinear assembly object
    MAST::StructuralModalEigenproblemAssembly   modal_assembly;
    _sys->initialize_condensed_dofs(*_discipline);
    
    modal_assembly.attach_discipline_and_system(*_discipline, *_structural_sys);
    _sys->eigenproblem_solve();
    modal_assembly.clear_discipline_and_system();
    
    // Get the number of converged eigen pairs.
    unsigned int
    nconv = std::min(_sys->get_n_converged_eigenvalues(),
                     _sys->get_n_requested_eigenvalues());
    if (_basis.size() > 0)
        libmesh_assert(_basis.size() == nconv);
    else {
        _basis.resize(nconv);
        for (unsigned int i=0; i<_basis.size(); i++)
            _basis[i] = NULL;
    }
    
    
    for (unsigned int i=0; i<nconv; i++) {
        
        // create a vector to store the basis
        if (_basis[i] == NULL)
            _basis[i] = _sys->solution->zero_clone().release();
        
        std::ostringstream file_name;
        
        // We write the file in the ExodusII format.
        file_name << "out_"
        << std::setw(3)
        << std::setfill('0')
        << std::right
        << i
        << ".exo";
        
        // now write the eigenvalue
        Real
        re = 0.,
        im = 0.;
        _sys->get_eigenpair(i, re, im, *_basis[i]);
        
        libMesh::out
        << std::setw(35) << std::fixed << std::setprecision(15)
        << re << std::endl;
        
        if (if_write_output) {
            
            std::cout
            << "Writing mode " << i << " to : "
            << file_name.str() << std::endl;
            
            // copy the solution for output
            (*_sys->solution) = *_basis[i];
            
            // We write the file in the ExodusII format.
            libMesh::ExodusII_IO(*_mesh).write_equation_systems(file_name.str(),
                                                                *_eq_sys);
        }
    }
    
    // now initialize the flutter solver
    MAST::StructuralFluidInteractionAssembly fsi_assembly;
    fsi_assembly.attach_discipline_and_system(*_discipline,
                                              *_structural_sys);
    _flutter_solver->attach_assembly(fsi_assembly);
    _flutter_solver->initialize(*_velocity,
                                0.0e3,        // lower V
                                9.0e3,        // upper V
                                10,           // number of divisions
                                _basis);      // basis vectors
//    _flutter_solver->scan_for_roots();
//    _flutter_solver->print_sorted_roots();
//    _flutter_solver->print_crossover_points();
    std::pair<bool, MAST::TimeDomainFlutterRootBase*>
    sol = _flutter_solver->analyze_and_find_critical_root_without_tracking(1.e-3, 20);
    _flutter_solver->print_sorted_roots();
    fsi_assembly.clear_discipline_and_system();
    _flutter_solver->clear_assembly_object();
    
    // make sure solution was found
    libmesh_assert(sol.first);
    _flutter_root = sol.second;

    if (if_write_output) {
        // now write the flutter mode to an output file.
        // Flutter mode Y = sum_i (X_i * (xi_re + xi_im)_i)
        // using the right eigenvector of the system.
        // where i is the structural mode
        //
        // The time domain simulation assumes the temporal solution to be
        // X(t) = (Y_re + i Y_im) exp(p t)
        //      = (Y_re + i Y_im) exp(p_re t) * (cos(p_im t) + i sin(p_im t))
        //      = exp(p_re t) (Z_re + i Z_im ),
        // where Z_re = Y_re cos(p_im t) - Y_im sin(p_im t), and
        //       Z_im = Y_re sin(p_im t) + Y_im cos(p_im t).
        //
        // We write the simulation of the mode over a period of oscillation
        //
        
        
        // first calculate the real and imaginary vectors
        std::auto_ptr<libMesh::NumericVector<Real> >
        re(_sys->solution->zero_clone().release()),
        im(_sys->solution->zero_clone().release());
        
        
        // first the real part
        _sys->solution->zero();
        for (unsigned int i=0; i<_basis.size(); i++) {
            re->add(sol.second->eig_vec_right(i).real(), *_basis[i]);
            im->add(sol.second->eig_vec_right(i).imag(), *_basis[i]);
        }
        re->close();
        im->close();
        
        // now open the output processor for writing
        libMesh::ExodusII_IO flutter_mode_output(*_mesh);
        
        // use N steps in a time-period
        Real
        t_sys = _sys->time,
        pi    = acos(-1.);
        unsigned int
        N_divs = 100;
        
        
        for (unsigned int i=0; i<=N_divs; i++) {
            _sys->time   =  2.*pi*(i*1.)/(N_divs*1.);
            
            _sys->solution->zero();
            _sys->solution->add( cos(_sys->time), *re);
            _sys->solution->add(-sin(_sys->time), *im);
            _sys->solution->close();
            flutter_mode_output.write_timestep("flutter_mode.exo",
                                               *_eq_sys,
                                               i+1,
                                               _sys->time);
        }
        
        // reset the system time
        _sys->time = t_sys;
    }
    
    return _flutter_root->V;
}





Real
MAST::PlateThermallyStressedPistonTheoryFlutterAnalysis::
sensitivity_solve(MAST::Parameter& p) {
    
    //Make sure that  a solution is available for sensitivity
    libmesh_assert(_flutter_root);
    
    // flutter solver will need velocity to be defined as a parameter for
    // sensitivity analysis
    _discipline->add_parameter(*_velocity);
    _discipline->add_parameter(p);
    
    libMesh::ParameterVector params;
    params.resize(1);
    params[0]  =  p.ptr();
    
    // initialize the flutter solver for sensitivity.
    MAST::StructuralFluidInteractionAssembly fsi_assembly;
    fsi_assembly.attach_discipline_and_system(*_discipline, *_structural_sys);
    _flutter_solver->attach_assembly(fsi_assembly);
    _flutter_solver->calculate_sensitivity(*_flutter_root, params, 0);
    fsi_assembly.clear_discipline_and_system();
    _flutter_solver->clear_assembly_object();
    
    _discipline->remove_parameter(p);
    _discipline->remove_parameter(*_velocity);
    return _flutter_root->V_sens;
}




void
MAST::PlateThermallyStressedPistonTheoryFlutterAnalysis::clear_stresss() {
    
    libmesh_assert(_initialized);
    
    // iterate over the output quantities and delete them
    std::vector<MAST::StressStrainOutputBase*>::iterator
    it   =   _outputs.begin(),
    end  =   _outputs.end();
    
    for ( ; it != end; it++)
        (*it)->clear(false);
}


