/*
 * MAST: Multidisciplinary-design Adaptation and Sensitivity Toolkit
 * Copyright (C) 2013-2017  Manav Bhatia
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
#include "base/system_initialization.h"
#include "base/nonlinear_system.h"

// libMesh includes
#include "libmesh/mesh_base.h"


MAST::SystemInitialization::SystemInitialization (MAST::NonlinearSystem& sys,
                                                  const std::string& prefix):
_system(sys),
_prefix(prefix) {

    // initialize the point locator for this mesh
    sys.system().get_mesh().sub_point_locator();
}



MAST::SystemInitialization::~SystemInitialization()
{ }




unsigned int
MAST::SystemInitialization::n_vars() const {
    return _system.n_vars();
}




const libMesh::FEType&
MAST::SystemInitialization::fetype(unsigned int i) const {
    
    return _system.variable_type(i);
}
