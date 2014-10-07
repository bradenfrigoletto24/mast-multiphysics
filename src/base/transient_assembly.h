
#ifndef __mast__transient_assembly__
#define __mast__transient_assembly__

// MAST includes
#include "base/assembly_base.h"

// libMesh includes
#include "libmesh/nonlinear_implicit_system.h"



namespace MAST {
    
    // Forward declerations
    class TransientSolverBase;
    
    
    class TransientAssembly:
    public MAST::AssemblyBase,
    public libMesh::NonlinearImplicitSystem::ComputeResidualandJacobian,
    public libMesh::System::SensitivityAssembly {
    public:
        
        /*!
         *   constructor associates this assembly object with the system
         */
        TransientAssembly(MAST::PhysicsDisciplineBase& assembly,
                          MAST::TransientSolverBase& solver,
                          MAST::SystemInitialization& sys);
        
        
        /*!
         *   destructor resets the association of this assembly object with
         *   the system
         */
        virtual ~TransientAssembly();
        
        
        /*!
         *    function that assembles the matrices and vectors quantities for
         *    nonlinear solution
         */
        virtual void
        residual_and_jacobian (const libMesh::NumericVector<Real>& X,
                               libMesh::NumericVector<Real>* R,
                               libMesh::SparseMatrix<Real>*  J,
                               libMesh::NonlinearImplicitSystem& S);
        
        /**
         * Assembly function.  This function will be called
         * to assemble the sensitivity of system residual prior to a solve and must
         * be provided by the user in a derived class. The method provides dR/dp_i
         * for \par i ^th parameter in the vector \par parameters.
         *
         * If the routine is not able to provide sensitivity for this parameter,
         * then it should return false, and the system will attempt to use
         * finite differencing.
         */
        virtual bool
        sensitivity_assemble (const libMesh::ParameterVector& parameters,
                              const unsigned int i,
                              libMesh::NumericVector<Real>& sensitivity_rhs);
        
        
        
        //**************************************************************
        //these methods are provided for use by the solvers
        //**************************************************************
        
        
        /*!
         *   performs the element calculations over \par elem, and returns
         *   the element vector and matrix quantities in \par mat and
         *   \par vec, respectively. \par if_jac tells the method to also
         *   assemble the Jacobian, in addition to the residual vector.
         */
        virtual void _elem_calculations(MAST::ElementBase& elem,
                                        bool if_jac,
                                        RealVectorX& x_dot,
                                        RealMatrixX& mass,
                                        RealMatrixX& jac) = 0;
        
        /*!
         *   performs the element sensitivity calculations over \par elem,
         *   and returns the element residual sensitivity in \par vec .
         */
        virtual void _elem_sensitivity_calculations(MAST::ElementBase& elem,
                                                    RealVectorX& vec) = 0;
        
        
    protected:

        /*!
         *   Pointer to a transient solver object that combines the
         *   element transient data appropriately for the nonlinear solver.
         */
        MAST::TransientSolverBase& _transient_solver;
    };
    
    
}

#endif // __mast__transient_assembly__