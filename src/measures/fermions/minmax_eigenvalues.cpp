#ifdef HAVE_CONFIG_H
 #include "config.hpp"
#endif

#include "dirac_operators/overlap/dirac_operator_overlap.hpp"
#include "eigenvalues/eigenvalues.hpp"
#include "geometry/geometry_mix.hpp"
#include "minmax_eigenvalues.hpp"
#include "new_types/rat_approx.hpp"
#include "operations/remez/remez_algorithm.hpp"

#ifdef USE_THREADS
 #include "routines/thread.hpp"
#endif

//////////////////////////////////////////////////////
////      C. BONANNO AND M.CARDINALI OVERLAP      ////
//////////////////////////////////////////////////////

namespace nissa
{
  //measure minmax_eigenvalues
  void measure_minmax_eigenvalues(quad_su3 **conf_eo,theory_pars_t &theory_pars,minmax_eigenvalues_meas_pars_t &meas_pars,int iconf,int conf_created)
  {
    FILE *file=open_file(meas_pars.path,conf_created?"w":"a");
    
    close_file(file);
    
    //lx version
    quad_su3 *conf_lx=nissa_malloc("conf_lx",loc_vol+bord_vol,quad_su3);
    paste_eo_parts_into_lx_vector(conf_lx,conf_eo);
    
    rat_approx_t appr;
    double maxerr=sqrt(meas_pars.residue);

    //Parameters of the eigensolver
    const int mat_size=loc_vol*NCOL*NDIRAC;
    const int mat_size_to_allocate=(loc_vol+bord_vol)*NCOL*NDIRAC;
    const int niter_max=100000000;
    master_printf("mat_size=%d, mat_size_to_allocate=%d\n",mat_size,mat_size_to_allocate);
    
    //allocate
    complex *D_ov_eig_val=nissa_malloc("D_ov_eig_val",meas_pars.neigs,complex);
    complex **eigvec=nissa_malloc("eigvec",meas_pars.neigs,complex*);
    for(int ieig=0;ieig<meas_pars.neigs;ieig++){
    eigvec[ieig]=nissa_malloc("eig",(loc_vol+bord_vol)*NCOL*NDIRAC,complex);
    vector_reset(eigvec[ieig]);}
    
    master_printf("neigs=%d, eig_precision=%.2e\n",meas_pars.neigs,meas_pars.eig_precision);
    
    //consider only the first quark
    int iquark=0;
    if(theory_pars.nflavs()!=1) crash("implemented only for 1 flavor");
    if(theory_pars.quarks[0].discretiz!=ferm_discretiz::OVERLAP) crash("Implemented only for overlap");
     
    rat_approx_for_overlap(conf_lx, &appr, theory_pars.quarks[iquark].mass_overlap, maxerr);
    
    appr.master_fprintf_expr(stdout);
 
    //Application of the Overlap Operator
    const auto imp_mat=[conf_lx,&theory_pars,&maxerr,iquark, &appr](complex* out_lx,complex *in_lx)
	{
	  apply_overlap((spincolor*)out_lx,conf_lx, &appr, maxerr, theory_pars.quarks[iquark].mass_overlap,(spincolor*)in_lx);
      	};
    const auto filler=[](complex *out_lx){generate_undiluted_source((spincolor*)out_lx,RND_GAUSS,-1);};
    
    double eig_time=-take_time();
    
    //Find eigenvalues and eigenvectors of the overlap
    eigenvalues_find((complex**)eigvec,D_ov_eig_val,meas_pars.neigs,meas_pars.min_max,mat_size,mat_size_to_allocate,imp_mat,meas_pars.eig_precision,niter_max,filler);
     
    master_printf("\n\nEigenvalues of D Overlap:\n");
    for(int ieig=0;ieig<meas_pars.neigs;++ieig)
    master_printf("%d(%.16lg,%.16lg)\n",ieig,D_ov_eig_val[ieig][RE],D_ov_eig_val[ieig][IM]);
    master_printf("\n\n\n");
    
    eig_time+=take_time();
    master_printf("Eigenvalues time: %lg\n", eig_time);
    
    for(int ieig=0;ieig<meas_pars.neigs;ieig++)
    master_fprintf(file,"%d %.16lg %.16lg\n",ieig, D_ov_eig_val[ieig][RE],D_ov_eig_val[ieig][IM]);
    master_fprintf(file, "\n\n\n");
    
     close_file(file);



    nissa_free(conf_lx);
    nissa_free(D_ov_eig_val);
    for(int ieig=0;ieig<meas_pars.neigs;ieig++) nissa_free(eigvec[ieig]);
    nissa_free(eigvec);
    
       
  }
  
  //print
  std::string minmax_eigenvalues_meas_pars_t::get_str(bool full)
  {
    std::ostringstream os;
    
    os<<"MeasMinMaxEigenval\n";
    os<<base_fermionic_meas_t::get_str(full);
    if(neigs!=def_neigs() or full) os<<" Neigs\t\t=\t"<<neigs<<"\n";
    if(eig_precision!=def_eig_precision() or full) os<<" EigPrecision\t\t=\t"<<eig_precision<<"\n";
    if(wspace_size!=def_wspace_size() or full) os<<" WSpaceSize\t\t=\t"<<wspace_size<<"\n";
    if(min_max!=def_min_max() or full) os<<" MinMax\t\t=\t"<<min_max<<"\n";
    
    return os.str();
  }
}
