/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Vincent Chabannes <vincent.chabannes@imag.fr>
       Date: 2011-07-17

  Copyright (C) 2011 Université Joseph Fourier (Grenoble I)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3.0 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file solidmecbase.hpp
   \author Vincent Chabannes <vincent.chabannes@feelpp.org>
   \date 2011-07-17
 */

#ifndef FEELPP_SOLIDMECHANICSBASE_HPP
#define FEELPP_SOLIDMECHANICSBASE_HPP 1


#include <feel/options.hpp>
#include <feel/feelalg/backend.hpp>
#include <feel/feeldiscr/functionspace.hpp>
#include <feel/feelfilters/gmsh.hpp>
#include <feel/feelfilters/exporter.hpp>
#include <feel/feelmesh/meshmover.hpp>
#include <feel/feelvf/vf.hpp>

#include <feel/feelts/bdf.hpp>
#include <feel/feelts/newmark.hpp>

#include <feel/feeldiscr/operatorinterpolation.hpp>

#include <feel/feelmodels/modelcore/modelnumerical.hpp>
#include <feel/feelmodels/modelcore/markermanagement.hpp>
#include <feel/feelmodels/solid/mechanicalpropertiesdescription.hpp>
#include <feel/feelmodels/modelcore/options.hpp>
#include <feel/feelmodels/modelalg/modelalgebraicfactory.hpp>

namespace Feel
{
namespace FeelModels
{
template< typename ConvexType, typename BasisDisplacementType,bool UseCstMechProp >
class SolidMechanicsBase : public ModelNumerical,
                           public MarkerManagementDirichletBC,
                           public MarkerManagementNeumannBC,
                           public MarkerManagementRobinBC,
                           public MarkerManagementFluidStructureInterfaceBC
{
public:
    typedef ModelNumerical super_type;

    typedef SolidMechanicsBase<ConvexType,BasisDisplacementType,UseCstMechProp> self_type;
    typedef boost::shared_ptr<self_type> self_ptrtype;

    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    // Standart Model
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    // mesh
    typedef ConvexType convex_type;
    static const uint16_type nDim = convex_type::nDim;
    static const uint16_type nOrderGeo = convex_type::nOrder;
    typedef Mesh<convex_type> mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;
    //___________________________________________________________________________________//
    // basis
    static const uint16_type nOrder = BasisDisplacementType::nOrder;//OrderDisp;
    static const uint16_type nOrderDisplacement = nOrder;
    static const uint16_type nOrderPressure = (nOrder>1)? nOrder-1:1;
    typedef BasisDisplacementType basis_u_type;
    typedef Lagrange<nOrderPressure, Scalar,Continuous,PointSetFekete> basis_l_type;
    typedef Lagrange<nOrder+1, Vectorial,Discontinuous,PointSetFekete> basis_stress_type;
    typedef Lagrange<0, Vectorial,Continuous> basis_constraint_vec_type;
    //___________________________________________________________________________________//
    // displacement space
    typedef FunctionSpace<mesh_type, bases<basis_u_type>/*,double,NoPeriodicity*/> space_displacement_type;
    typedef boost::shared_ptr<space_displacement_type> space_displacement_ptrtype;
    typedef typename space_displacement_type::element_type element_displacement_type;
    typedef boost::shared_ptr<element_displacement_type> element_displacement_ptrtype;
    typedef typename space_displacement_type::element_type element_vectorial_type;
    typedef boost::shared_ptr<element_vectorial_type> element_vectorial_ptrtype;
    //___________________________________________________________________________________//
    // pressure space
    typedef FunctionSpace<mesh_type, bases<basis_l_type> > space_pressure_type;
    typedef boost::shared_ptr<space_pressure_type> space_pressure_ptrtype;
    typedef typename space_pressure_type::element_type element_pressure_type;
    typedef boost::shared_ptr<element_pressure_type> element_pressure_ptrtype;
    //___________________________________________________________________________________//
    // vectorial constraint space
    typedef FunctionSpace<mesh_type, bases<basis_constraint_vec_type> > space_constraint_vec_type;
    typedef boost::shared_ptr<space_constraint_vec_type> space_constraint_vec_ptrtype;
    typedef typename space_constraint_vec_type::element_type element_constraint_vec_type;
    typedef boost::shared_ptr<element_constraint_vec_type> element_constraint_vec_ptrtype;
    //___________________________________________________________________________________//
    // scalar stress space
    typedef Lagrange<nOrder, Scalar,Continuous,PointSetFekete> basis_stress_scal_type;
    typedef FunctionSpace<mesh_type, bases<basis_stress_scal_type>/*, double, NoPeriodicity*/> space_stress_scal_type;
    typedef boost::shared_ptr<space_stress_scal_type> space_stress_scal_ptrtype;
    typedef typename space_stress_scal_type::element_type element_stress_scal_type;
    typedef boost::shared_ptr<element_stress_scal_type> element_stress_scal_ptrtype;
    // vectorial stress space
    typedef FunctionSpace<mesh_type, bases<basis_stress_type>/*, double, NoPeriodicity*/> space_stress_type;
    typedef boost::shared_ptr<space_stress_type> space_stress_ptrtype;
    typedef typename space_stress_type::element_type element_stress_type;
    typedef boost::shared_ptr<element_stress_type> element_stress_ptrtype;
    //___________________________________________________________________________________//
    // methodsnum tool
    typedef ModelAlgebraicFactory model_algebraic_factory_type;
    typedef boost::shared_ptr< model_algebraic_factory_type > model_algebraic_factory_ptrtype;
    typedef typename model_algebraic_factory_type::graph_type graph_type;
    typedef typename model_algebraic_factory_type::graph_ptrtype graph_ptrtype;
    //___________________________________________________________________________________//
    // newmark or savets(bdf) class
    typedef Newmark<space_displacement_type> newmark_displacement_type;
    typedef boost::shared_ptr<newmark_displacement_type> newmark_displacement_ptrtype;
    typedef Bdf<space_pressure_type>  savets_pressure_type;
    typedef boost::shared_ptr<savets_pressure_type> savets_pressure_ptrtype;
    //___________________________________________________________________________________//
    // functionspace for rho,coefflame1,coefflame2
    typedef bases<Lagrange<0, Scalar,Continuous> > basis_scalar_P0_continuous_type;
    typedef bases<Lagrange<0, Scalar,Discontinuous> > basis_scalar_P0_discontinuous_type;
    typedef typename mpl::if_< mpl::bool_<UseCstMechProp>,
                               basis_scalar_P0_continuous_type,
                               basis_scalar_P0_discontinuous_type >::type basis_scalar_P0_type;

    typedef FunctionSpace<mesh_type, basis_scalar_P0_type> space_scalar_P0_type;
    typedef boost::shared_ptr<space_scalar_P0_type> space_scalar_P0_ptrtype;
    typedef typename space_scalar_P0_type::element_type element_scalar_P0_type;
    typedef boost::shared_ptr<element_scalar_P0_type> element_scalar_P0_ptrtype;
    // mechanical properties desc
    typedef MechanicalPropertiesDescription<space_scalar_P0_type> mechanicalproperties_type;
    typedef boost::shared_ptr<mechanicalproperties_type> mechanicalproperties_ptrtype;
    //___________________________________________________________________________________//
    // trace mesh
    typedef typename mesh_type::trace_mesh_type trace_mesh_type;
    typedef boost::shared_ptr<trace_mesh_type> trace_mesh_ptrtype;
    typedef Lagrange<nOrderGeo, Vectorial,Continuous> basis_tracemesh_disp_type;
    typedef FunctionSpace<trace_mesh_type, bases<basis_tracemesh_disp_type> > space_tracemesh_disp_type;
    typedef boost::shared_ptr<space_tracemesh_disp_type> space_tracemesh_disp_ptrtype;
    typedef typename space_tracemesh_disp_type::element_type element_tracemesh_disp_type;
    typedef boost::shared_ptr<element_tracemesh_disp_type> element_tracemesh_disp_ptrtype;
    //___________________________________________________________________________________//
    // exporter
    typedef Exporter<mesh_type,nOrderGeo> exporter_type;
    typedef boost::shared_ptr<exporter_type> exporter_ptrtype;
    //typedef Exporter<mesh_type,nOrderGeo> gmsh_export_type;
    //typedef boost::shared_ptr<gmsh_export_type> gmsh_export_ptrtype;
#if defined(FEELPP_HAS_VTK)
    //fais comme ca car bug dans opeartorlagrangeP1 pour les champs vectorielles
    typedef FunctionSpace<mesh_type,bases<Lagrange<nOrder,Scalar,Continuous,PointSetFekete> > > space_create_ho_type;

    typedef Mesh<Simplex<nDim,1,nDim> > mesh_visu_ho_type;

    typedef FunctionSpace<mesh_visu_ho_type,bases<Lagrange<1,Vectorial,Continuous,PointSetFekete> > > space_vectorial_visu_ho_type;
    typedef boost::shared_ptr<space_vectorial_visu_ho_type> space_vectorial_visu_ho_ptrtype;
    typedef typename space_vectorial_visu_ho_type::element_type element_vectorial_visu_ho_type;
    typedef boost::shared_ptr<element_vectorial_visu_ho_type> element_vectorial_visu_ho_ptrtype;


    typedef boost::tuple<boost::mpl::size_t<MESH_ELEMENTS>,
                         typename MeshTraits<mesh_visu_ho_type>::element_const_iterator,
                         typename MeshTraits<mesh_visu_ho_type>::element_const_iterator> range_visu_ho_type;
    typedef boost::tuple<boost::mpl::size_t<MESH_FACES>,
                         typename MeshTraits<mesh_visu_ho_type>::location_face_const_iterator,
                         typename MeshTraits<mesh_visu_ho_type>::location_face_const_iterator> range_visu_boundaryfaces_ho_type;

    typedef OperatorInterpolation<space_displacement_type,
                                  space_vectorial_visu_ho_type,
                                  range_visu_ho_type> op_interpolation_visu_ho_disp_type;
    typedef boost::shared_ptr<op_interpolation_visu_ho_disp_type> op_interpolation_visu_ho_disp_ptrtype;

    typedef OperatorInterpolation<space_stress_type,
                                  space_vectorial_visu_ho_type,
                                  range_visu_boundaryfaces_ho_type/*range_visu_ho_type*/> op_interpolation_visu_ho_normalstress_type;
    typedef boost::shared_ptr<op_interpolation_visu_ho_normalstress_type> op_interpolation_visu_ho_normalstress_ptrtype;

    //typedef FunctionSpace<mesh_visu_ho_type,bases<Lagrange<1,Scalar,Continuous,PointSetFekete> > > space_scalar_visu_ho_type;
    typedef typename space_vectorial_visu_ho_type::component_functionspace_type space_scalar_visu_ho_type;
    typedef boost::shared_ptr<space_scalar_visu_ho_type> space_scalar_visu_ho_ptrtype;
    typedef typename space_scalar_visu_ho_type::element_type element_scalar_visu_ho_type;
    typedef boost::shared_ptr<element_scalar_visu_ho_type> element_scalar_visu_ho_ptrtype;

    typedef OperatorInterpolation<space_pressure_type,
                                  space_scalar_visu_ho_type,
                                  range_visu_ho_type> op_interpolation_visu_ho_pressure_type;
    typedef boost::shared_ptr<op_interpolation_visu_ho_pressure_type> op_interpolation_visu_ho_pressure_ptrtype;

    typedef Exporter<mesh_visu_ho_type> export_ho_type;
    typedef boost::shared_ptr<export_ho_type> export_ho_ptrtype;
#endif
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //1d reduced_part
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    // mesh
    typedef Simplex<1,1/*nOrderGeo*/,nDim> convex_1dreduced_type;
    typedef Mesh<convex_1dreduced_type> mesh_1dreduced_type;
    typedef boost::shared_ptr<mesh_1dreduced_type> mesh_1dreduced_ptrtype;
    //___________________________________________________________________________________//
    // basis
    //typedef bases<Lagrange<nOrder, Scalar,Continuous,PointSetFekete> > basis_1dreduced_type;
    typedef bases<Lagrange<nOrder, Vectorial,Continuous,PointSetFekete> > basis_vect_1dreduced_type;
    typedef bases<Lagrange<nOrder+1, Vectorial,Discontinuous,PointSetFekete> > basis_stress_vect_1dreduced_type;
    //___________________________________________________________________________________//
    // function space
    typedef FunctionSpace<mesh_1dreduced_type, basis_vect_1dreduced_type> space_vect_1dreduced_type;
    typedef boost::shared_ptr<space_vect_1dreduced_type> space_vect_1dreduced_ptrtype;
    typedef typename space_vect_1dreduced_type::element_type element_vect_1dreduced_type;
    typedef boost::shared_ptr<element_vect_1dreduced_type> element_vect_1dreduced_ptrtype;

    typedef typename space_vect_1dreduced_type::component_functionspace_type space_1dreduced_type;
    typedef typename space_vect_1dreduced_type::component_functionspace_ptrtype space_1dreduced_ptrtype;
    typedef typename space_1dreduced_type::element_type element_1dreduced_type;
    typedef boost::shared_ptr<element_1dreduced_type> element_1dreduced_ptrtype;

    typedef FunctionSpace<mesh_1dreduced_type, basis_stress_vect_1dreduced_type> space_stress_vect_1dreduced_type;
    typedef boost::shared_ptr<space_stress_vect_1dreduced_type> space_stress_vect_1dreduced_ptrtype;
    typedef typename space_stress_vect_1dreduced_type::element_type element_stress_vect_1dreduced_type;
    typedef boost::shared_ptr<element_stress_vect_1dreduced_type> element_stress_vect_1dreduced_ptrtype;

    typedef typename space_stress_vect_1dreduced_type::component_functionspace_type space_stress_scal_1dreduced_type;
    typedef typename space_stress_vect_1dreduced_type::component_functionspace_ptrtype space_stress_scal_1dreduced_ptrtype;
    typedef typename space_stress_scal_1dreduced_type::element_type element_stress_scal_1dreduced_type;
    typedef boost::shared_ptr<element_stress_scal_1dreduced_type> element_stress_scal_1dreduced_ptrtype;
    //___________________________________________________________________________________//
    // time step newmark
    typedef Newmark<space_1dreduced_type>  newmark_1dreduced_type;
    typedef boost::shared_ptr<newmark_1dreduced_type> newmark_1dreduced_ptrtype;
    //___________________________________________________________________________________//
    // exporter
    typedef Exporter<mesh_1dreduced_type,mesh_1dreduced_type::nOrder> exporter_1dreduced_type;
    typedef boost::shared_ptr<exporter_1dreduced_type> exporter_1dreduced_ptrtype;
    //___________________________________________________________________________________//

    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    // interpolation between 1d and 2d/3d
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//

    typedef boost::tuple<boost::mpl::size_t<MESH_FACES>,
                         typename MeshTraits<mesh_type>::marker_face_const_iterator,
                         typename MeshTraits<mesh_type>::marker_face_const_iterator> range_face_type;
    typedef OperatorInterpolation<space_vect_1dreduced_type, space_displacement_type, range_face_type> op_interpolation1dTo2d_disp_type;
    typedef boost::shared_ptr<op_interpolation1dTo2d_disp_type> op_interpolation1dTo2d_disp_ptrtype;

    typedef boost::tuple<boost::mpl::size_t<MESH_ELEMENTS>,
                         typename MeshTraits<mesh_1dreduced_type>::element_const_iterator,
                         typename MeshTraits<mesh_1dreduced_type>::element_const_iterator> range_elt1d_reduced_type;
    typedef OperatorInterpolation<space_stress_scal_type, space_1dreduced_type ,range_elt1d_reduced_type> op_interpolation2dTo1d_normalstress_type;
    typedef boost::shared_ptr<op_interpolation2dTo1d_normalstress_type> op_interpolation2dTo1d_normalstress_ptrtype;

    //___________________________________________________________________________________//

    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//
    //___________________________________________________________________________________//


    SolidMechanicsBase( std::string prefix,
                        bool __buildMesh=true,
                        WorldComm const& _worldComm=Environment::worldComm(),
                        std::string subPrefix="",
                        std::string appliShortRepository=soption(_name="exporter.directory") );
    SolidMechanicsBase( self_type const & M ) = default;

    static std::string expandStringFromSpec( std::string const& expr );

    void build();
    void loadMesh(mesh_ptrtype __mesh );
    void loadMesh(mesh_1dreduced_ptrtype __mesh );
    //protected :
    void build(mesh_ptrtype mesh );
    void build( mesh_1dreduced_ptrtype mesh );
protected :
    void buildStandardModel( mesh_ptrtype mesh = mesh_ptrtype() );
    void build1dReducedModel( mesh_1dreduced_ptrtype mesh = mesh_1dreduced_ptrtype() );

    virtual void loadConfigMeshFile(std::string const& geofilename) = 0;
    virtual void loadConfigMeshFile1dReduced(std::string const& geofilename) = 0;
    void loadParameterFromOptionsVm();
    void createWorldsComm();

    void createMesh();
    void createMesh1dReduced();
    void createFunctionSpaces();
    void createFunctionSpaces1dReduced();
    void createTimeDiscretisation();
    void createTimeDiscretisation1dReduced();
    void createExporters();
    void createExporters1dReduced();
    void createOthers();

    void createAdditionalFunctionSpacesFSI();
    void createAdditionalFunctionSpacesFSIStandard();
    void createAdditionalFunctionSpacesFSI1dReduced();

public :
    void restartExporters() { this->restartExporters( this->timeInitial() ); }
    void restartExporters1dReduced() { this->restartExporters1dReduced( this->timeInitial() ); }
    void restartExporters( double time );
    void restartExporters1dReduced( double time );


    std::string fileNameMeshPath() const { return prefixvm(this->prefix(),"SolidMechanicsMesh.path"); }

    //-----------------------------------------------------------------------------------//

    void init( bool buildAlgebraicFactory, typename model_algebraic_factory_type::appli_ptrtype const& app );
    virtual void solve( bool upVelAcc=true );

    boost::shared_ptr<std::ostringstream> getInfo() const;

    void pdeType(std::string __type);
    void pdeSolver(std::string __type);

    bool is1dReducedModel() const { return M_is1dReducedModel; }
    bool isStandardModel() const { return M_isStandardModel; }
    bool useDisplacementPressureFormulation() const { return M_useDisplacementPressureFormulation; }
    void setUseDisplacementPressureFormulation( bool b )
    {
        M_useDisplacementPressureFormulation = b;
        if (M_mechanicalProperties) M_mechanicalProperties->setUseDisplacementPressureFormulation(b);
    }

    //-----------------------------------------------------------------------------------//
    // all model
    //-----------------------------------------------------------------------------------//

    mechanicalproperties_ptrtype const& mechanicalProperties() const { return M_mechanicalProperties; }

    boost::shared_ptr<TSBase> timeStepBase()
    {
        if (this->isStandardModel())
            return this->timeStepNewmark();
        else// if (this->is1dReducedModel())
            return this->timeStepNewmark1dReduced();
    }
    boost::shared_ptr<TSBase> timeStepBase() const
    {
        if (this->isStandardModel())
            return this->timeStepNewmark();
        else// if (this->is1dReducedModel())
            return this->timeStepNewmark1dReduced();
    }

    void updateTimeStep();
    void updateVelocity();

    void exportResults() { this->exportResults( this->currentTime() ); }
    void exportResults( double time );
private :
    void exportResultsImpl( double time );
    void exportResultsImplHO( double time );
public :

    void predictorDispl();

    block_pattern_type blockPattern() const;

    backend_ptrtype const& backend() const
    {
        if (this->isStandardModel())
            return this->backendStandard();
        else// if (this->is1dReducedModel())
            return this->backend1dReduced();
    }

    //-----------------------------------------------------------------------------------//
    // standart model
    //-----------------------------------------------------------------------------------//

    mesh_ptrtype const& mesh() const { return M_mesh; }

    space_displacement_ptrtype const& functionSpace() const { return this->functionSpaceDisplacement(); }
    space_displacement_ptrtype const& functionSpaceDisplacement() const { return M_Xh; }
    space_pressure_ptrtype const& functionSpacePressure() const { CHECK( M_XhPressure ) << "space pressure not define";return M_XhPressure; }

    element_displacement_type & fieldDisplacement() { return *M_fieldDisplacement; }
    element_displacement_type const& fieldDisplacement() const { return *M_fieldDisplacement; }
    element_displacement_ptrtype & fieldDisplacementPtr() { return M_fieldDisplacement; }
    element_displacement_ptrtype const& fieldDisplacementPtr() const { return M_fieldDisplacement; }
    element_pressure_type & fieldPressure() { CHECK( M_fieldPressure ) << "field pressure not define"; return *M_fieldPressure; }
    element_pressure_type const& fieldPressure() const { CHECK( M_fieldPressure ) << "field pressure not define"; return *M_fieldPressure; }
    element_pressure_ptrtype & fieldPressurePtr() { CHECK( M_fieldPressure ) << "field pressure not define"; return M_fieldPressure; }
    element_pressure_ptrtype const& fieldPressurePtr() const { CHECK( M_fieldPressure ) << "field pressure not define"; return M_fieldPressure; }

    newmark_displacement_ptrtype & timeStepNewmark() { return M_newmark_displ_struct; }
    newmark_displacement_ptrtype const& timeStepNewmark() const { return M_newmark_displ_struct; }
    savets_pressure_ptrtype const& timeStepSavetsPressure() const { CHECK( M_savetsPressure ) << "savets pressure not define"; return M_savetsPressure; }

    element_displacement_ptrtype & fieldVelocityPtr() { return M_newmark_displ_struct->currentVelocityPtr(); }
    element_displacement_ptrtype const& fieldVelocityPtr() const { return M_newmark_displ_struct->currentVelocityPtr(); }
    element_displacement_type & fieldVelocity() { return M_newmark_displ_struct->currentVelocity(); }
    element_displacement_type const& fieldVelocity() const { return M_newmark_displ_struct->currentVelocity(); }

    element_stress_ptrtype normalStressFromFluid() { return M_normalStressFromFluid; }
    element_stress_ptrtype const& normalStressFromFluid() const { return M_normalStressFromFluid; }
    element_stress_ptrtype normalStressFromStruct() { return M_normalStressFromStruct; }
    element_stress_ptrtype const& normalStressFromStruct() const { return M_normalStressFromStruct; }
    element_vectorial_ptrtype velocityInterfaceFromFluid() { return M_velocityInterfaceFromFluid; }
    element_vectorial_ptrtype const& velocityInterfaceFromFluid() const { return M_velocityInterfaceFromFluid; }

    //----------------------------------//
    //backend_ptrtype backend() { return M_backend; }
    backend_ptrtype const& backendStandard() const { return M_backend; }

    BlocksBaseGraphCSR buildBlockMatrixGraph() const;
    graph_ptrtype buildMatrixGraph() const;
    int nBlockMatrixGraph() const;
    std::map<std::string,size_type> const& startDofIndexFieldsInMatrix() const { return M_startDofIndexFieldsInMatrix; }
    BlocksBaseVector<double> blockVectorSolution() { return M_blockVectorSolution; }
    BlocksBaseVector<double> const& blockVectorSolution() const { return M_blockVectorSolution; }
    void updateBlockVectorSolution();


    //-----------------------------------------------------------------------------------//
    // 1d reduced model
    //-----------------------------------------------------------------------------------//

    //mesh_1dreduced_ptrtype mesh1dReduced() { return M_mesh_1dReduced; }
    mesh_1dreduced_ptrtype const& mesh1dReduced() const { return M_mesh_1dReduced; }
    space_1dreduced_ptrtype functionSpace1dReduced() { return M_Xh_1dReduced; }
    space_1dreduced_ptrtype const& functionSpace1dReduced() const { return M_Xh_1dReduced; }

    element_1dreduced_type & fieldDisplacementScal1dReduced() { return *M_disp_1dReduced; }
    element_1dreduced_type const & fieldDisplacementScal1dReduced() const { return *M_disp_1dReduced; }
    element_vect_1dreduced_type & fieldDisplacementVect1dReduced() { return *M_disp_vect_1dReduced; }
    element_vect_1dreduced_type const & fieldDisplacementVect1dReduced() const { return *M_disp_vect_1dReduced; }
    element_vect_1dreduced_ptrtype const & fieldDisplacementVect1dReducedPtr() const { return M_disp_vect_1dReduced; }

    element_stress_scal_1dreduced_type & fieldStressScal1dReduced() { return *M_stress_1dReduced; }
    element_stress_vect_1dreduced_type & fieldStressVect1dReduced() { return *M_stress_vect_1dReduced; }
    element_stress_vect_1dreduced_type const& fieldStressVect1dReduced() const { return *M_stress_vect_1dReduced; }
    element_stress_vect_1dreduced_type const& fieldStressVect1dReducedPtr() const { return *M_stress_vect_1dReduced; }
    element_1dreduced_type & fieldVelocityScal1dReduced() { return *M_velocity_1dReduced; }
    element_1dreduced_type const& fieldVelocityScal1dReduced() const { return *M_velocity_1dReduced; }
    element_vect_1dreduced_type & fieldVelocityVect1dReduced() { return *M_velocity_vect_1dReduced; }
    element_vect_1dreduced_type const& fieldVelocityVect1dReduced() const { return *M_velocity_vect_1dReduced; }
    element_vect_1dreduced_ptrtype const& fieldVelocityVect1dReducedPtr() const { return M_velocity_vect_1dReduced; }

    newmark_1dreduced_ptrtype & timeStepNewmark1dReduced() { return M_newmark_displ_1dReduced; }
    newmark_1dreduced_ptrtype const& timeStepNewmark1dReduced() const { return M_newmark_displ_1dReduced; }

    backend_ptrtype const& backend1dReduced() const { return M_backend_1dReduced; }

    double thickness1dReduced() const { return M_thickness_1dReduced; }
    double radius1dReduced() const { return M_radius_1dReduced; }

    //-----------------------------------------------------------------------------------//

    void TransfertDisp1dTo2d(std::string __rangename);
    void TransfertStress2dTo1d(std::string __rangename);
    void precomputeTransfertStress2dTo1d(std::string __rangename);
    void precomputeTransfertDisp1dTo2d(std::string __rangename);
#if 0
    void precomputeDisp1dTo2dWithInterpolation();
    void precomputeNormalStress2dTo1dWithInterpolation();
    void transfertDisp1dTo2dWithInterpolation();
    void transfertNormalStress2dTo1dWithInterpolation();
#endif


    // fsi coupling
    bool useFSISemiImplicitScheme() const { return M_useFSISemiImplicitScheme; }
    void useFSISemiImplicitScheme(bool b) { M_useFSISemiImplicitScheme=b; }
    std::string couplingFSIcondition() const { return M_couplingFSIcondition; }
    void couplingFSIcondition(std::string s) { M_couplingFSIcondition=s; }

    void updateSubMeshDispFSIFromPrevious();

    //std::list<std::string> getMarkerNameFSI() const { return M_markerNameFSI; }
    std::list<std::string> const& markerNameFSI() const { return this->markerFluidStructureInterfaceBC(); }
    double gammaNitschFSI() const { return M_gammaNitschFSI; }
    void gammaNitschFSI(double d) { M_gammaNitschFSI=d; }
    double muFluidFSI() const { return M_muFluidFSI; }
    void muFluidFSI(double d) { M_muFluidFSI=d; }




    void updateNormalStressFromStruct();

    void updatePreStress() { *U_displ_struct_prestress=*M_fieldDisplacement; }


    template <typename element_fluid_ptrtype>
    void updateStressTensor(element_fluid_ptrtype fluidSol);

    void updateStressTensorBis(element_stress_ptrtype stressN);

    template <typename element_fluid_ptrtype>
    void updateVelocityInterface(element_fluid_ptrtype fluidSol);

    //usefull for 1d reduced model
    void updateInterfaceDispFrom1dDisp();
    void updateInterfaceVelocityFrom1dVelocity();
    void updateInterfaceScalStressDispFromVectStress();

    element_vect_1dreduced_ptrtype
    extendVelocity1dReducedVectorial( element_1dreduced_type const& vel1d ) const;

    //-----------------------------------------------------------------------------------//
    // post processing computation
    //-----------------------------------------------------------------------------------//
    void saveDataInPoint(const std::list<boost::tuple<std::string,typename mesh_type::node_type> > & __listPt, bool extrapolate=false);

    double computeMaxDisp() const;
    double computeMaxDispOnBoundary(std::string __marker) const;

    double computeIncompressibility() const;


    //-----------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------//
    //-----------------------------------------------------------------------------------//

    // assembly methods
    virtual void updateNewtonInitialGuess(vector_ptrtype& U) const = 0;

    void updateLinearPDE(const vector_ptrtype& X, sparse_matrix_ptrtype& A, vector_ptrtype& F,
                         bool _buildCstPart,
                         sparse_matrix_ptrtype& A_extended, bool _BuildExtendedPart,
                         bool _doClose=true,
                         bool _doBCStrongDirichlet=true ) const;

    void updateLinearGeneralisedString(const vector_ptrtype& X,sparse_matrix_ptrtype& A , vector_ptrtype& F) const;
    void updateLinearGeneralisedStringGeneralisedAlpha(const vector_ptrtype& X,sparse_matrix_ptrtype& A , vector_ptrtype& F,
                                                       bool _buildCstPart,
                                                       bool _doClose=true,
                                                       bool _doBCStrongDirichlet=true) const;

    //void updateLinearElasticity(const vector_ptrtype& X,sparse_matrix_ptrtype& A , vector_ptrtype& F);
    void updateLinearElasticityGeneralisedAlpha(const vector_ptrtype& X,sparse_matrix_ptrtype& A , vector_ptrtype& F,
                                                bool _buildCstPart,
                                                bool _doClose=true,
                                                bool _doBCStrongDirichlet=true) const;
    void updateLinearElasticityAxiSymGeneralisedAlpha(const vector_ptrtype& X,sparse_matrix_ptrtype& A , vector_ptrtype& F,
                                                      bool _buildCstPart,
                                                      bool _doClose=true,
                                                      bool _doBCStrongDirichlet=true) const;

    void updateJacobian( const vector_ptrtype& X, sparse_matrix_ptrtype& J, vector_ptrtype& R,
                         bool BuildCstPart, sparse_matrix_ptrtype& A_extended, bool _BuildExtendedPart,
                         bool _doClose=true, bool _doBCStrongDirichlet=true ) const;
    void updateResidual( const vector_ptrtype& X, vector_ptrtype& R,
                         bool BuildCstPart, bool UseJacobianLinearTerms,
                         bool _doClose=true, bool _doBCStrongDirichlet=true ) const;

    void updateJacobianIncompressibilityTerms( element_displacement_type const& u, element_pressure_type const& p, sparse_matrix_ptrtype& J) const;
    void updateResidualIncompressibilityTerms( element_displacement_type const& u, element_pressure_type const& p, vector_ptrtype& R) const;

    void updateJacobianViscoElasticityTerms( element_displacement_type const& u, sparse_matrix_ptrtype& J) const;
    void updateResidualViscoElasticityTerms( element_displacement_type const& u, vector_ptrtype& R) const;


    virtual void updateBCDirichletStrongResidual(vector_ptrtype& R) const = 0;
    virtual void updateBCNeumannResidual( vector_ptrtype& R ) const = 0;
    virtual void updateBCRobinResidual( element_displacement_type const& u, vector_ptrtype& R ) const = 0;
    virtual void updateBCFollowerPressureResidual(element_displacement_type const& u, vector_ptrtype& R ) const = 0;
    virtual void updateSourceTermResidual( vector_ptrtype& R ) const = 0;

    virtual void updateBCDirichletStrongJacobian(sparse_matrix_ptrtype& J) const = 0;
    virtual void updateBCFollowerPressureJacobian(element_displacement_type const& u, sparse_matrix_ptrtype& J) const = 0;
    virtual void updateBCRobinJacobian( sparse_matrix_ptrtype& J) const = 0;

    virtual void updateBCDirichletStrongLinearPDE(sparse_matrix_ptrtype& A, vector_ptrtype& F) const = 0;
    virtual void updateBCNeumannLinearPDE( vector_ptrtype& F ) const = 0;
    virtual void updateBCRobinLinearPDE( sparse_matrix_ptrtype& A, vector_ptrtype& F ) const = 0;
    virtual void updateSourceTermLinearPDE( vector_ptrtype& F ) const = 0;

protected:

    // model
    std::string M_pdeType,M_pdeSolver;
    bool M_useDisplacementPressureFormulation;
    bool M_is1dReducedModel;
    bool M_isStandardModel;

    bool M_hasBuildFromMesh,M_hasBuildFromMesh1dReduced, M_isUpdatedForUse;


    //model parameters
    space_scalar_P0_ptrtype M_XhScalarP0;
    mechanicalproperties_ptrtype M_mechanicalProperties;

    //-------------------------------------------//
    // standart model
    //-------------------------------------------//

    // mesh
    double M_meshSize;
    mesh_ptrtype M_mesh;
    MeshMover<mesh_type> M_mesh_mover;
    MeshMover<typename mesh_type::trace_mesh_type> M_meshMoverTrace;
    // function space
    space_displacement_ptrtype M_Xh;
    element_displacement_ptrtype M_fieldDisplacement;
    space_pressure_ptrtype M_XhPressure;
    element_pressure_ptrtype M_fieldPressure;
    space_constraint_vec_ptrtype M_XhConstraintVec;
    element_constraint_vec_ptrtype M_fieldConstraintVec;
    space_displacement_ptrtype M_XhVectorial;
    element_vectorial_ptrtype U_displ_struct_prestress;
    element_vectorial_ptrtype M_velocityInterfaceFromFluid;
    // normal stress space
    space_stress_ptrtype M_XhStress;
    space_stress_scal_ptrtype M_XhScalarStress;
    element_stress_ptrtype M_normalStressFromFluid;
    element_stress_ptrtype M_normalStressFromStruct;
    // time discretisation
    newmark_displacement_ptrtype M_newmark_displ_struct;
    savets_pressure_ptrtype M_savetsPressure;
    // algebraic solver ( assembly+solver )
    model_algebraic_factory_ptrtype M_algebraicFactory;
    // start dof index fields in matrix (lm,windkessel,...)
    std::map<std::string,size_type> M_startDofIndexFieldsInMatrix;
    // backend
    backend_ptrtype M_backend;
    // block vector solution
    BlocksBaseVector<double> M_blockVectorSolution;
    // trace mesh
    space_tracemesh_disp_ptrtype M_XhSubMeshDispFSI;
    element_tracemesh_disp_ptrtype M_fieldSubMeshDispFSI;

    // exporter
    exporter_ptrtype M_exporter;
    bool M_doExportDisplacement, M_doExportVelocity, M_doExportAcceleration, M_doExportNormalStress, M_doExportPressure;
    bool M_doExportVelocityInterfaceFromFluid;
    bool M_isHOVisu;
#if defined(FEELPP_HAS_VTK)
    export_ho_ptrtype M_exporter_ho;
    space_vectorial_visu_ho_ptrtype M_XhVectorialVisuHO;
    element_vectorial_visu_ho_ptrtype M_displacementVisuHO;
    op_interpolation_visu_ho_disp_ptrtype M_opIdisplacement;
    op_interpolation_visu_ho_normalstress_ptrtype M_opInormalstress;
    space_scalar_visu_ho_ptrtype M_XhScalarVisuHO;
    element_scalar_visu_ho_ptrtype M_pressureVisuHO;
    op_interpolation_visu_ho_pressure_ptrtype M_opIpressure;
#endif

    //-------------------------------------------//
    // 1d_reduced model
    //-------------------------------------------//

    // mesh
    mesh_1dreduced_ptrtype M_mesh_1dReduced;
    // function space
    space_1dreduced_ptrtype M_Xh_1dReduced;
    space_stress_vect_1dreduced_ptrtype M_XhStressVect_1dReduced;
    //element disp,vel,acc
    element_1dreduced_ptrtype M_disp_1dReduced;
    element_1dreduced_ptrtype M_velocity_1dReduced;
    element_1dreduced_ptrtype M_acceleration_1dReduced;
    // normal stress space
    element_stress_scal_1dreduced_ptrtype M_stress_1dReduced;
    // vectorial 1d_reduced space
    space_vect_1dreduced_ptrtype M_Xh_vect_1dReduced;
    element_vect_1dreduced_ptrtype M_disp_vect_1dReduced;
    element_vect_1dreduced_ptrtype M_velocity_vect_1dReduced;
    element_stress_vect_1dreduced_ptrtype M_stress_vect_1dReduced;
    // time discretisation
    newmark_1dreduced_ptrtype M_newmark_displ_1dReduced;
    // backend
    backend_ptrtype M_backend_1dReduced;
    // algebraic solver ( assembly+solver )
    model_algebraic_factory_ptrtype M_algebraicFactory_1dReduced;
    // exporter
    exporter_1dreduced_ptrtype M_exporter_1dReduced;

    // axi-sym properties
    double M_thickness_1dReduced, M_radius_1dReduced;

    //-------------------------------------------//
    // others
    //-------------------------------------------//

    //generalised-alpha parameters
    double M_genAlpha_rho;
    double M_genAlpha_alpha_m;
    double M_genAlpha_alpha_f;
    double M_genAlpha_gamma;
    double M_genAlpha_beta;

    //std::list<std::string> M_markerNameBCRobin;
    // fsi
    bool M_useFSISemiImplicitScheme;
    std::string M_couplingFSIcondition;
    //std::list<std::string> M_markerNameFSI;
    double M_gammaNitschFSI;
    double M_muFluidFSI;
    boost::shared_ptr<typename mesh_type::trace_mesh_type> M_fsiSubmesh;


    std::set<std::string> M_nameFilesData;


}; // SolidMechanics


} // FeelModels
} // Feel

#endif /* FEELPP_SOLIDMECHANICSBASE_HPP */