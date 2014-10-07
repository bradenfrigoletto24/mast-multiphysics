//
//  physics_discipline_base.h
//  mast
//
//  Created by Manav Bhatia on 9/24/14.
//  Copyright (c) 2014 MAST. All rights reserved.
//

#ifndef __mast__physics_discipline_base__
#define __mast__physics_discipline_base__

// C++ includes
#include <map>

// MAST includes
#include "base/mast_data_types.h"

// libMesh includes
#include "libmesh/equation_systems.h"


namespace MAST {
    
    // Forward declerations
    class BoundaryConditionBase;
    class DirichletBoundaryCondition;
    class PropertyCardBase;
    class FunctionBase;
    class FunctionSetBase;
    template <typename T> class ConstantFunction;
    class SystemInitialization;
    class Parameter;
    
    // typedefs
    typedef std::multimap<libMesh::boundary_id_type, MAST::BoundaryConditionBase*>  SideBCMapType;
    typedef std::multimap<libMesh::subdomain_id_type, MAST::BoundaryConditionBase*> VolumeBCMapType;
    typedef std::map<libMesh::subdomain_id_type, const MAST::FunctionSetBase*>      PropertyCardMapType;
    typedef std::map<libMesh::boundary_id_type, MAST::DirichletBoundaryCondition*>  DirichletBCMapType;

    
    class PhysicsDisciplineBase {
    public:
        
        
        // Constructor
        PhysicsDisciplineBase(libMesh::EquationSystems& eq_sys):
        _eq_systems(eq_sys)
        { }
        
        /*!
         *   virtual destructor
         */
        virtual ~PhysicsDisciplineBase()
        { }
        
        
        /*!
         *   returns a reference to the libMesh::System object
         */
        libMesh::EquationSystems& get_equation_systems() {
            return _eq_systems;
        }
        
        /*!
         *   clear the loads and pointer to static solution system for
         *   this structural model
         */
        void clear_loads();
        
        /*!
         *   clear the specified volume load from the applied loads
         */
        void clear_volume_load(libMesh::subdomain_id_type bid,
                               MAST::BoundaryConditionBase& load);
        
        /*!
         *   adds the specified side loads for the boudnary with tag \p b_id
         */
        void add_side_load(libMesh::boundary_id_type bid,
                           MAST::BoundaryConditionBase& load);
        
        /*!
         *  adds the specified Dirichlet boundary condition for the boundary
         *  with tag \p b_id
         */
        void add_dirichlet_bc(libMesh::boundary_id_type bid,
                              MAST::DirichletBoundaryCondition& load);
        
        /*!
         *    @returns a const reference to the side boundary conditions
         */
        const MAST::SideBCMapType& side_loads() const {
            return _side_bc_map;
        }
        
        
        /*!
         *    @returns a reference to the side boundary conditions
         */
        MAST::SideBCMapType& side_loads() {
            return _side_bc_map;
        }
        
        /*!
         *   adds the specified volume loads for the elements with
         *   subdomain tag \p s_id
         */
        void add_volume_load(libMesh::subdomain_id_type bid,
                             MAST::BoundaryConditionBase& load);
        
        
        /*!
         *    @returns a const reference to the bolume boundary conditions
         */
        const MAST::VolumeBCMapType& volume_loads() const{
            return _vol_bc_map;
        }
        
        /*!
         *    @returns a reference to the bolume boundary conditions
         */
        MAST::VolumeBCMapType& volume_loads() {
            return _vol_bc_map;
        }
        
        /*!
         *    initializes the system for dirichlet boundary conditions
         */
        template <typename SysType>
        void init_system_dirichlet_bc(MAST::SystemInitialization& sys) const;
        
        /*!
         *    sets the same property for all elements in the specified subdomain
         */
        void set_property_for_subdomain(const libMesh::subdomain_id_type sid,
                                        const MAST::FunctionSetBase& prop);
        
        /*!
         *    get the material property for the specified element
         */
        const MAST::FunctionSetBase& get_property_card(const libMesh::Elem& elem) const;
        
        /*!
         *    get the material property for the specified subdomain id \par i
         */
        const MAST::FunctionSetBase& get_property_card(const unsigned int i) const;
        
        /*!
         *   Adds the parameter and function pairing
         */
        void add_parameter(MAST::Parameter& f);
        
        /*!
         *   Returns the function corresponding to a parameter
         */
        const MAST::FunctionBase* get_parameter(Real* par) const;
        
        
    protected:
        
        /*!
         *    libMesh::System for which analysis is to be performed
         */
        libMesh::EquationSystems& _eq_systems;
        
        /*!
         *   map of element property cards for each element
         */
        MAST::PropertyCardMapType _element_property;
        
        /*!
         *   map of sensitivity parameters and the corresponding functions that
         *   are directly dependent on these parameters
         */
        std::map<Real*, const MAST::FunctionBase*> _parameter_map;
        
        /*!
         *   side boundary condition map of boundary id and load
         */
        MAST::SideBCMapType _side_bc_map;
        
        /*!
         *   Dirichlet boundary condition map of boundary id and load
         */
        MAST::DirichletBCMapType _dirichlet_bc_map;
        
        /*!
         *   volume boundary condition map of boundary id and load
         */
        MAST::VolumeBCMapType _vol_bc_map;
    };
    
}


#endif // __mast__physics_discipline_base__