//! -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*- vim:fenc=utf-8:ft=cpp:et:sw=4:ts=4:sts=4
//!
//! This file is part of the Feel++ library
//!
//! This library is free software; you can redistribute it and/or
//! modify it under the terms of the GNU Lesser General Public
//! License as published by the Free Software Foundation; either
//! version 2.1 of the License, or (at your option) any later version.
//!
//! This library is distributed in the hope that it will be useful,
//! but WITHOUT ANY WARRANTY; without even the implied warranty of
//! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//! Lesser General Public License for more details.
//!
//! You should have received a copy of the GNU Lesser General Public
//! License along with this library; if not, write to the Free Software
//! Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//!
//! @file
//! @author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
//! @date 10 Apr 2017
//! @copyright 2017 Feel++ Consortium
//!
// tag::global[]
#include <feel/feel.hpp>
#include <tabulate/table.hpp>
#include <feel/feelpython/pyexpr.hpp>
#include <feel/feelvf/print.hpp>
#include "cg_laplacian.hpp"
int main( int argc, char** argv )
{
    // tag::env[]
    using namespace Feel;
    using namespace tabulate;
    using Feel::cout;

    po::options_description laplacianoptions( "Laplacian options" );
    
    laplacianoptions.add_options()( "no-solve", po::value<bool>()->default_value( false ), "No solve" )
        ( "k", po::value<std::string>()->default_value( "1" ), "diffusion coefficient" )
        ( "f", po::value<std::string>()->default_value( "" ), "right hand side" )
        ( "g", po::value<std::string>()->default_value( "" ), "Dirichlet boundary condition" )
        ( "un", po::value<std::string>()->default_value( "" ), "Neumann boundary condition" )
        ( "r_1", po::value<std::string>()->default_value( "1" ), "Robin left hand side coefficient" )
        ( "r_2", po::value<std::string>()->default_value( "" ), "Robin right hand side  coefficient" )
        ( "pyexpr.filename", po::value<std::string>()->default_value( "$cfgdir/../python/laplacian.py" ), "python filename to execute" );
    laplacianoptions.add_options()( "marker.name", po::value<std::string>(), "marker on which to solve problem" );
    laplacianoptions.add_options()( "marker.levelset", po::value<std::string>(), "marker on which to solve problem" );

    Environment env( _argc = argc, _argv = argv,
                     _desc = laplacianoptions,
                     _about = about( _name = "qs_laplacian",
                                     _author = "Feel++ Consortium",
                                     _email = "feelpp-devel@feelpp.org" ) );

    // end::env[]

    // tag::mesh_space[]
    tic();
#if FEELPP_HYPERCUBE == 1
    using mesh_t = Mesh<Hypercube<FEELPP_DIM, 1>>;
    auto mesh = loadMesh( _mesh = new mesh_t );
#else
    using mesh_t = Mesh<Simplex<FEELPP_DIM, 1>>;
    auto mesh = loadMesh( _mesh = new mesh_t );
#endif
    toc( "loadMesh" );

    tic();
    Pch_ptrtype<mesh_t,2> Vh;
    if ( Environment::vm().count("marker.name") )
        Vh = Pch<2>( mesh, markedelements(mesh, soption("marker.name") ) );
    else if ( Environment::vm().count("marker.levelset") )
        Vh = Pch<2>( mesh, elements(mesh, expr(soption("marker.levelset")) ) );
    else
        Vh = Pch<2>( mesh );


    // if we do not check the results with a manufactured solution,
    // the right hand side is given by functions.f otherwise it is computed by the python script
    auto thechecker = checker( "L1/H1 convergence", soption("checker.solution") );
    auto check_data = [&thechecker]( std::string data, std::string helper, bool defined = true ) {
                          using namespace std::string_literals;
                          if ( defined )
                          {
                              if ( !thechecker.check() && data.empty() )
                                  throw std::invalid_argument( "Invalid data "s + data + " "s + helper);
                          }
                      };
    try {
        check_data( soption( _name = "k" ), " set diffusion coefficient using option --k (eg. --k=1)");
        check_data( soption( _name = "f" ), " set source term using option --f (eg. --f=1)");
        check_data( soption( _name = "g" ), " set Dirichlet condition using option --g (eg. --g=0)",  support( Vh )->hasAnyMarker( {"Dirichlet"} ) );
        check_data( soption( _name = "un" ), " set Neumann condition using option --un (eg. --un=0)", support( Vh )->hasAnyMarker( {"Neumann"} ) );
        check_data( soption( _name = "r_2" ), " set Robin right hand side condition using option --r_2 (eg. --r_2=0)", support( Vh )->hasAnyMarker( {"Robin"} ) );
    }
    catch( std::invalid_argument const& e )
    {
        Feel::cout << e.what() << std::endl;
        Feel::cout << "Exiting..." << std::endl;
        return 1;
    }
    auto opt = [&thechecker]( std::string data ) {
                   using namespace std::string_literals;
                   if ( thechecker.check() )
                       return ""s;
                   return data;
              };
    std::map<std::string,std::string> locals{{"dim",std::to_string(FEELPP_DIM)},{"k",opt(soption("k"))},{"p",soption("checker.solution")},{"grad_p",""}, {"u",""}, {"un",opt(soption("un"))}, {"f",opt(soption("f"))}, {"g",opt(soption("g"))}, {"r_1",soption("r_1")}, {"r_2",opt(soption("r_2"))}};
    
    Feel::pyexprFromFile( Environment::expand(soption("pyexpr.filename")), locals );

    std::string p_exact_str = locals.at("p");
    std::string u_exact_str = locals.at("u");
    auto p_exact = expr( p_exact_str );
    auto u_exact = expr<FEELPP_DIM,1>( u_exact_str );
    auto k = expr( locals.at("k") );
    auto un = expr( locals.at("un") );
    auto f = expr( locals.at("f") );
    auto g = expr( locals.at("g") );
    auto r_1 = expr( locals.at("r_1") );
    auto r_2 = expr( locals.at("r_2") );
    thechecker.setSolution( locals.at("p") );

    Table summary;
    summary.add_row({"Solving -div(( k grad p ) = f with the following boundary conditions"});
    summary[0].format().font_align(FontAlign::center);
    Table data;
    //data.format().hide_border();
    data.add_row({"k",locals.at("k")});
    data.add_row({"f",locals.at("f")});
    
    if ( support( Vh )->hasAnyMarker( {"Dirichlet"} ) ) 
    {
        data.add_row({"Dirichlet BC",std::to_string(nelements( markedfaces( support( Vh ), "Dirichlet" ) ))});
        data.add_row({"p", locals.at("g")}); 
    }
    if ( support( Vh )->hasAnyMarker( {"Neumann"} ) ) 
    {
        data.add_row({"Neumann BC",std::to_string(nelements( markedfaces( support( Vh ), "Neumann" ) ))});
        data.add_row({"-k*dn(p)", locals.at("un")}); 
    }
    if ( support( Vh )->hasAnyMarker( {"Robin"} ) ) 
    {
        data.add_row({"Robin BC",std::to_string(nelements( markedfaces( support( Vh ), "Robin" ) ))});
        data.add_row({"-k*dn(p)+"+locals.at("r_1")+"*u", locals.at("r_2")}); 
    }
    data.column(0).format().font_align(FontAlign::right);
    summary.add_row({data});
    if ( thechecker.check() )
    {
        Table ex;
        //ex.format().hide_border();
        summary.add_row({"Exact solution to be checked with L2/H1 norms"});
        ex.add_row({"p",locals.at("p")});
        ex.add_row({"-k*grad(p)",locals.at("u")});
        ex.column(0).format().font_align(FontAlign::right);
        summary.add_row({ex});
    }
    Feel::cout << summary << std::endl;
    if ( Environment::isMasterRank() )
    {
        std::ofstream ofs("model.txt" );
        ofs << summary << std::endl;
    }


    auto u = cgLaplacian( Vh, std::tuple{k,f,g,un,r_1,r_2} );

    // tag::export[]
    tic();
    auto e = exporter( _mesh = mesh );
    e->addRegions();
    e->add( "p", u );
    e->add( "u", -k*gradv(u), "element" );
    e->add( "k", k );
    e->add( "f", f );
    e->add( "g", g );
    e->add( "r_1", r_1 );
    e->add( "r_2", r_2 );
    if ( thechecker.check() )
    {
        e->add( "solution", p_exact );
        e->add( "flux", u_exact );
    }
    e->save();
    toc( "Exporter" );
    // end::export[]

    int status = 0;
    // tag::check[]
    if ( thechecker.check() )
    {
        // compute l2 and h1 norm of u-u_h where u=solution
        auto norms = [=]( std::string const& solution ) -> std::map<std::string, double> {
            tic();
            double l2 = normL2( _range = elements( support( Vh ) ), _expr = idv( u ) - expr( solution ) );
            toc( "L2 error norm" );
            tic();
            double h1 = normH1( _range = elements( support( Vh ) ), _expr = idv( u ) - expr( solution ), _grad_expr = gradv( u ) - grad<FEELPP_DIM>( expr( solution ) ) );
            toc( "H1 error norm" );
            return {{"L2", l2}, {"H1", h1}};
        };

        status = !thechecker.runOnce( norms, rate::hp( mesh->hMax(), Vh->fe()->order() ) );
    }
    // end::check[]

    // exit status = 0 means no error
    return status;
}
// end::global[]
