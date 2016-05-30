#ifndef _CG_INVERT_TMCLOVQ2_64_HPP
#define _CG_INVERT_TMCLOVQ2_64_HPP

#include "new_types/su3.hpp"

#include "cg_64_invert_tmclovQ2_bgq.hpp"
#include "cg_64_invert_tmclovQ2_portable.hpp"

namespace nissa
{
  inline void inv_tmclovQ2_cg_64(spincolor *sol,spincolor *guess,quad_su3 *conf,double kappa,double csw,as2t_su3 *Pmunu,double mu,int niter,double residue,spincolor *source)
  {
#if defined BGQ
    //bufferize and remap
    bi_oct_su3 *bi_conf=nissa_malloc("bi_conf",loc_volh,bi_oct_su3);
    lx_conf_remap_to_virlx(bi_conf,conf);
    bi_spincolor *bi_source=nissa_malloc("bi_source",loc_volh,bi_spincolor);
    lx_spincolor_remap_to_virlx(bi_source,source);
    bi_spincolor *bi_sol=nissa_malloc("bi_sol",loc_volh,bi_spincolor);
    
    inv_tmQ2_m2_cgm_bgq(bi_sol,bi_conf,kappa,m2,nmass,niter_max,req_res,bi_source);
    
    //unmap and free
    virlx_spincolor_remap_to_lx(sol,bi_sol);
    nissa_free(bi_sol);
    nissa_free(bi_source);
    nissa_free(bi_conf);
#else
    inv_tmclovQ2_cg_64_portable(sol,guess,conf,kappa,csw,Pmunu,mu,niter,residue,source);
#endif
  }
}

#endif
