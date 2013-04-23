#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include "../../communicate/communicate.h"
#include "../../base/global_variables.h"
#include "../../base/vectors.h"
#include "../../geometry/geometry_mix.h"
#include "../../linalgs/linalgs.h"
#include "../../new_types/new_types_definitions.h"
#include "../../new_types/su3.h"
#include "../../operations/shift.h"
#include "../../routines/ios.h"
#include "../../routines/thread.h"

#include "../smearing/APE.h"
#include "../smearing/HYP.h"

//compute all possible rectangular paths among a defined interval
THREADABLE_FUNCTION_4ARG(measure_all_rectangular_paths, all_rect_meas_pars_t*,pars, quad_su3*,ori_conf, int,iconf, int,conf_created)
{
  GET_THREAD_ID();
  
  FILE *fout=NULL;
  if(rank==0 && thread_id==0) fout=open_file(pars->path,conf_created?"w":"a");
  
  //hypped and APE spatial smeared conf
  quad_su3 *sme_conf=nissa_malloc("sme_conf",loc_vol+bord_vol+edge_vol,quad_su3);
  
  //allocate time path, time-spatial paths, closing paths and point contribution
  su3 *T_path=nissa_malloc("T_path",loc_vol+bord_vol,su3);
  su3 *TS_path=nissa_malloc("TS_path",loc_vol+bord_vol,su3);
  su3 *closed_path=nissa_malloc("closed_path",loc_vol+bord_vol,su3);
  double *point_path=nissa_malloc("point_path",loc_vol,double);
  
  //hyp the conf
  hyp_smear_conf_dir(sme_conf,ori_conf,pars->hyp_alpha0,pars->hyp_alpha1,pars->hyp_alpha2,0);

  //loop over APE smeared levels
  for(int iape=0;iape<pars->nape_levls;iape++)
    {
      //APE smearing
      ape_spatial_smear_conf(sme_conf,sme_conf,pars->ape_alpha,
			     (iape==0)?pars->nape_iters[0]:(pars->nape_iters[iape]-pars->nape_iters[iape-1]));
      
      //reset the Tpath link product
      NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	su3_put_to_id(T_path[ivol]);
      
      //move along T up to Tmax
      for(int t=0;t<pars->Tmax;t++)
	{
	  //take the product
	  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	    safe_su3_prod_su3(T_path[ivol],T_path[ivol],sme_conf[ivol][0]);
	  set_borders_invalid(T_path);
      
	  //push up the vector along T
	  su3_vec_single_shift(T_path,0,+1);
	  
	  //results to be printed, averaged along the three dirs
	  double paths[pars->Dmax];
	  for(int d=0;d<pars->Dmax;d++) paths[d]=0;
	  
	  //if T_path is long enough we move along spatial dirs
	  if(t>=pars->Tmin)
	    for(int i=1;i<4;i++)
	      {
		//copy T_path
		NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
		  su3_copy(TS_path[ivol],T_path[ivol]);
	    
		//move along i up to Dmax
		for(int d=0;d<pars->Dmax;d++)
		  {
		    //take the product
		    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
		      safe_su3_prod_su3(TS_path[ivol],TS_path[ivol],sme_conf[ivol][i]);
		    set_borders_invalid(TS_path);

		    //push up the vector along i
		    su3_vec_single_shift(TS_path,i,+1);

		    //if TS_path is long enough we close the path
		    if(d>=pars->Dmin)
		      {
			//copy TS_path
			NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
			  su3_copy(closed_path[ivol],TS_path[ivol]);
			set_borders_invalid(closed_path);
			
			//move back along time
			for(int tp=0;tp<=t;tp++)
			  {
			    //push dw the vector along 0
			    su3_vec_single_shift(closed_path,0,-1);
			    
			    //take the product with dag
			    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
			      safe_su3_prod_su3_dag(closed_path[ivol],closed_path[ivol],sme_conf[ivol][0]);
			    set_borders_invalid(closed_path);
			  }

			//move back along space
			for(int dp=0;dp<=d;dp++)
			  {
			    //push dw the vector along i
			    su3_vec_single_shift(closed_path,i,-1);
			    
			    //take the product with dag
			    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
			      safe_su3_prod_su3_dag(closed_path[ivol],closed_path[ivol],sme_conf[ivol][i]);
			    set_borders_invalid(closed_path);
			  }
			
			//take the trace and store it in the point contribution
			NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
			  point_path[ivol]=
			      closed_path[ivol][0][0][RE]+closed_path[ivol][1][1][RE]+closed_path[ivol][2][2][RE];
			
			//reduce among all threads and ranks and summ it
			double temp;
			double_vector_glb_collapse(&temp,point_path,loc_vol);
			paths[d]+=temp;
		      }
		  }
	      }
	  
	  //print all the Dmax contributions, with ncol*nspat_dir*glb_vol normalization
	  if(rank==0 && thread_id==0)
	    for(int d=pars->Dmin;d<pars->Dmax;d++)
	      fprintf(fout,"%d %d  %d %d %16.16lg\n",iconf,iape,t,d,paths[d]/(9*glb_vol));
	}
    }
  
  //close file
  if(rank==0 && thread_id==0) fclose(fout);
  
  //free stuff
  nissa_free(sme_conf);
  nissa_free(T_path);
  nissa_free(TS_path);
  nissa_free(closed_path);
  nissa_free(point_path);
}}

void measure_all_rectangular_paths(all_rect_meas_pars_t *pars,quad_su3 **conf_eo,int iconf,int conf_created)
{
  quad_su3 *conf_lx=nissa_malloc("conf_lx",loc_vol+bord_vol+edge_vol,quad_su3);
  paste_eo_parts_into_lx_conf(conf_lx,conf_eo);
  
  measure_all_rectangular_paths(pars,conf_lx,iconf,conf_created);
  
  nissa_free(conf_lx);
}