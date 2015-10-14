#ifndef _STOUT_HPP
#define _STOUT_HPP

namespace nissa
{
  void stouted_force_compute_Lambda(su3 Lambda,su3 U,su3 F,anti_hermitian_exp_ingredients *ing);
  //eo
  void stout_smear_whole_stack(quad_su3 ***out,quad_su3 **in,stout_pars_t *stout_pars);
  void stout_smear(quad_su3 **ext_out,quad_su3 **ext_in,stout_pars_t *stout_pars);
  void stout_smear_single_level(quad_su3 **out,quad_su3 **ext_in, double rho);
  void stout_smear_compute_staples(stout_link_staples *out,quad_su3 **conf,int p,int A,int mu,double rho);
  void stout_smear_compute_weighted_staples(su3 staples,quad_su3 **conf,int p,int A,int mu,double rho);
  void stout_smear_conf_stack_allocate(quad_su3 ****out,quad_su3 **in,int nlev);
  void stout_smear_conf_stack_free(quad_su3 ****out,int nlev);
  void stouted_force_compute_Lambda(su3 Lambda,su3 U,su3 F,anti_hermitian_exp_ingredients *ing);
  void stouted_force_remap(quad_su3 **F,quad_su3 ***sme_conf,stout_pars_t *stout_pars);
  void stouted_force_remap_step(quad_su3 **F,quad_su3 **conf,double rho);
  //lx
  void stout_smear_whole_stack(quad_su3 **out,quad_su3 *in,stout_pars_t *stout_pars);
  void stout_smear(quad_su3 *ext_out,quad_su3 *ext_in,stout_pars_t *stout_pars);
  void stout_smear_single_level(quad_su3 *out,quad_su3 *ext_in,double *rho);
  void stout_smear_compute_staples(stout_link_staples *out,quad_su3 *conf,int p,int A,int mu,double rho);
  void stout_smear_compute_weighted_staples(su3 staples,quad_su3 *conf,int p,int A,int mu,double rho);
  void stout_smear_conf_stack_allocate(quad_su3 ***out,quad_su3 *in,int nlev);
  void stout_smear_conf_stack_free(quad_su3 ***out,int nlev);
  void stouted_force_compute_Lambda(su3 Lambda,su3 U,su3 F,anti_hermitian_exp_ingredients *ing);
  void stouted_force_remap(quad_su3 *F,quad_su3 **sme_conf,stout_pars_t *stout_pars);
  void stouted_force_remap_step(quad_su3 *F,quad_su3 *conf,double rho);
}

#endif
