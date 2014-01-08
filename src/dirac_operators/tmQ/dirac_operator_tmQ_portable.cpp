#pragma once

#include "routines/ios.hpp"

namespace nissa
{
  //Apply the Q=D*g5 operator to a spincolor
  THREADABLE_FUNCTION_5ARG(apply_tmQ, spincolor*,out, quad_su3*,conf, double,kappa, double,mu, spincolor*,in)
  {
    if(!check_borders_valid(conf)) communicate_lx_quad_su3_borders(conf);
    if(!check_borders_valid(in)) communicate_lx_spincolor_borders(in);
    
    double kcf=1/(2*kappa);
    
    GET_THREAD_ID();
    NISSA_PARALLEL_LOOP(X,0,loc_vol)
      {
	int Xup,Xdw;
	color temp_c0,temp_c1,temp_c2,temp_c3;
	
	//Forward 0
	Xup=loclx_neighup[X][0];
	color_summ(temp_c0,in[Xup][0],in[Xup][2]);
	color_summ(temp_c1,in[Xup][1],in[Xup][3]);
	unsafe_su3_prod_color(out[X][0],conf[X][0],temp_c0);
	unsafe_su3_prod_color(out[X][1],conf[X][0],temp_c1);
	color_copy(out[X][2],out[X][0]);
	color_copy(out[X][3],out[X][1]);
	
	//Backward 0
	Xdw=loclx_neighdw[X][0];
	color_subt(temp_c0,in[Xdw][0],in[Xdw][2]);
	color_subt(temp_c1,in[Xdw][1],in[Xdw][3]);
	unsafe_su3_dag_prod_color(temp_c2,conf[Xdw][0],temp_c0);
	unsafe_su3_dag_prod_color(temp_c3,conf[Xdw][0],temp_c1);
	color_summassign(out[X][0],temp_c2);
	color_summassign(out[X][1],temp_c3);
	color_subtassign(out[X][2],temp_c2);
	color_subtassign(out[X][3],temp_c3);
	
	//Forward 1
	Xup=loclx_neighup[X][1];
	color_isumm(temp_c0,in[Xup][0],in[Xup][3]);
	color_isumm(temp_c1,in[Xup][1],in[Xup][2]);
	unsafe_su3_prod_color(temp_c2,conf[X][1],temp_c0);
	unsafe_su3_prod_color(temp_c3,conf[X][1],temp_c1);
	color_summassign(out[X][0],temp_c2);
	color_summassign(out[X][1],temp_c3);
	color_isubtassign(out[X][2],temp_c3);
	color_isubtassign(out[X][3],temp_c2);
	
	//Backward 1
	Xdw=loclx_neighdw[X][1];
	color_isubt(temp_c0,in[Xdw][0],in[Xdw][3]);
	color_isubt(temp_c1,in[Xdw][1],in[Xdw][2]);
	unsafe_su3_dag_prod_color(temp_c2,conf[Xdw][1],temp_c0);
	unsafe_su3_dag_prod_color(temp_c3,conf[Xdw][1],temp_c1);
	color_summassign(out[X][0],temp_c2);
	color_summassign(out[X][1],temp_c3);
	color_isummassign(out[X][2],temp_c3);
	color_isummassign(out[X][3],temp_c2);
	
	//Forward 2
	Xup=loclx_neighup[X][2];
	color_summ(temp_c0,in[Xup][0],in[Xup][3]);
	color_subt(temp_c1,in[Xup][1],in[Xup][2]);
	unsafe_su3_prod_color(temp_c2,conf[X][2],temp_c0);
	unsafe_su3_prod_color(temp_c3,conf[X][2],temp_c1);
	color_summassign(out[X][0],temp_c2);
	color_summassign(out[X][1],temp_c3);
	color_subtassign(out[X][2],temp_c3);
	color_summassign(out[X][3],temp_c2);
	
	//Backward 2
	Xdw=loclx_neighdw[X][2];
	color_subt(temp_c0,in[Xdw][0],in[Xdw][3]);
	color_summ(temp_c1,in[Xdw][1],in[Xdw][2]);
	unsafe_su3_dag_prod_color(temp_c2,conf[Xdw][2],temp_c0);
	unsafe_su3_dag_prod_color(temp_c3,conf[Xdw][2],temp_c1);
	color_summassign(out[X][0],temp_c2);
	color_summassign(out[X][1],temp_c3);
	color_summassign(out[X][2],temp_c3);
	color_subtassign(out[X][3],temp_c2);
	
	//Forward 3
	Xup=loclx_neighup[X][3];
	color_isumm(temp_c0,in[Xup][0],in[Xup][2]);
	color_isubt(temp_c1,in[Xup][1],in[Xup][3]);
	unsafe_su3_prod_color(temp_c2,conf[X][3],temp_c0);
	unsafe_su3_prod_color(temp_c3,conf[X][3],temp_c1);
	color_summassign(out[X][0],temp_c2);
	color_summassign(out[X][1],temp_c3);
	color_isubtassign(out[X][2],temp_c2);
	color_isummassign(out[X][3],temp_c3);
	
	//Backward 3
	Xdw=loclx_neighdw[X][3];
	color_isubt(temp_c0,in[Xdw][0],in[Xdw][2]);
	color_isumm(temp_c1,in[Xdw][1],in[Xdw][3]);
	unsafe_su3_dag_prod_color(temp_c2,conf[Xdw][3],temp_c0);
	unsafe_su3_dag_prod_color(temp_c3,conf[Xdw][3],temp_c1);
	color_summassign(out[X][0],temp_c2);
	color_summassign(out[X][1],temp_c3);
	color_isummassign(out[X][2],temp_c2);
	color_isubtassign(out[X][3],temp_c3);
	
	//Put the -1/2 factor on derivative, the gamma5, and the imu
	//ok this is horrible, but fast
	for(int c=0;c<3;c++)
	  {
	    out[X][0][c][0]=-0.5*out[X][0][c][0]+kcf*in[X][0][c][0]-mu*in[X][0][c][1];
	    out[X][0][c][1]=-0.5*out[X][0][c][1]+kcf*in[X][0][c][1]+mu*in[X][0][c][0];
	    out[X][1][c][0]=-0.5*out[X][1][c][0]+kcf*in[X][1][c][0]-mu*in[X][1][c][1];
	    out[X][1][c][1]=-0.5*out[X][1][c][1]+kcf*in[X][1][c][1]+mu*in[X][1][c][0];
	    out[X][2][c][0]=+0.5*out[X][2][c][0]-kcf*in[X][2][c][0]-mu*in[X][2][c][1];
	    out[X][2][c][1]=+0.5*out[X][2][c][1]-kcf*in[X][2][c][1]+mu*in[X][2][c][0];
	    out[X][3][c][0]=+0.5*out[X][3][c][0]-kcf*in[X][3][c][0]-mu*in[X][3][c][1];
	    out[X][3][c][1]=+0.5*out[X][3][c][1]-kcf*in[X][3][c][1]+mu*in[X][3][c][0];
	  }
      }
    
    set_borders_invalid(out);
  }}
}