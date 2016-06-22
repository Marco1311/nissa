#include <nissa.hpp>

#include "conf.hpp"
#include "contr.hpp"
#include "pars.hpp"
#include "prop.hpp"

using namespace nissa;

double M_of_mom(tm_quark_info qu,double sin2_momh)
{
  double M=m0_of_kappa(qu.kappa)+2*sin2_momh;
  return M;
}

double M_of_mom(tm_quark_info qu,int imom)
{
  momentum_t sin_mom;
  double sin2_mom,sin2_momh;
  get_component_of_twisted_propagator_of_imom(sin_mom,sin2_mom,sin2_momh,qu,imom);
  return M_of_mom(qu,sin2_momh);
}

double mom_comp_of_coord(int ip_mu,tm_quark_info qu,int mu)
{return M_PI*(2*ip_mu+qu.bc[mu])/glb_size[mu];}

double mom_comp_of_site(int ip,tm_quark_info qu,int mu)
{return mom_comp_of_coord(glb_coord_of_loclx[ip][mu],qu,mu);}

void sin_mom(momentum_t sin_mom,int imom,tm_quark_info qu)
{for(int mu=0;mu<NDIM;mu++) sin_mom[mu]=sin(mom_comp_of_coord(glb_coord_of_loclx[imom][mu],qu,mu));}

double sin2_mom(int imom,tm_quark_info qu)
{double out=0;for(int mu=0;mu<NDIM;mu++) out+=sqr(sin(mom_comp_of_coord(glb_coord_of_loclx[imom][mu],qu,mu)));return out;}

double den_of_mom(int imom,tm_quark_info qu)
{
  //takes the momenta part
  momentum_t sin_mom;
  double sin2_mom,sin2_momh;
  get_component_of_twisted_propagator_of_imom(sin_mom,sin2_mom,sin2_momh,qu,imom);
  
  //compute M and the denominator
  double M=M_of_mom(qu,sin2_momh);
  double den=sin2_mom+sqr(M)+sqr(qu.mass);
  
  return den;
}

double mom_prod(momentum_t p,momentum_t q)
{
  double pro=0;
  for(int mu=0;mu<NDIM;mu++) pro+=p[mu]*q[mu];
  return pro;
}

void bar_transf(complex *co,tm_quark_info qu)
{
  //for(int p=0;p<glb_size[0];p++)
  //master_printf("%d %.16lg %.16lg\n",p,co[p][0],co[p][1]);
  //master_printf("\n");
  
  tm_quark_info ba=qu;
  ba.bc[0]=QUARK_BOUND_COND*3;
  
  complex c[glb_size[0]];
  for(int x=0;x<glb_size[0];x++)
    {
      complex_put_to_zero(c[x]);
      for(int ip0=0;ip0<glb_size[0];ip0++)
	{
	  double p0=mom_comp_of_coord(ip0,ba,0);
	  complex fact={cos(p0*x),sin(p0*x)};
	  if(x<glb_size[0]/2) complex_summ_the_prod(c[x],co[ip0],fact);
	  else                complex_summ_the_conj1_prod(c[x],co[ip0],fact);
	}
      master_printf("%d %.16lg %.16lg\n",x,c[x][0],c[x][1]);
    }
}

void bar_contr_free(complex *mess,tm_quark_info qu)
{
  double mu2=qu.mass*qu.mass;
  complex co[glb_size[0]];
  memset(co,0,sizeof(complex)*glb_size[0]);
  NISSA_LOC_VOL_LOOP(p)
    {
      double Mp=M_of_mom(qu,p);
      double dp=den_of_mom(p,qu);
      momentum_t sin_p;
      sin_mom(sin_p,p,qu);
      
      NISSA_LOC_VOL_LOOP(q)
        {
	  double Mq=M_of_mom(qu,q);
	  double dq=den_of_mom(q,qu);
	  momentum_t sin_q;
	  sin_mom(sin_q,q,qu);
	  
	  double pq=mom_prod(sin_p,sin_q);
	  double numden=-NCOL*8*(mu2-Mp*Mq-pq)/(dp*dq*glb_vol*glb_size[0]);
	  
	  for(int r0=0;r0<glb_size[0];r0++)
	    {
	      coords cr;
	      cr[0]=(3*glb_size[0]+r0-glb_coord_of_loclx[p][0]-glb_coord_of_loclx[q][0])%glb_size[0];
	      for(int mu=1;mu<NDIM;mu++) cr[mu]=(3*glb_size[mu]-glb_coord_of_loclx[p][mu]-glb_coord_of_loclx[q][mu])%glb_size[mu];
	      int r=loclx_of_coord(cr);
	      
	      complex_summ_the_prod_double(co[r0],mess[r],numden);
	    }
	}
    }
  
  bar_transf(co,qu);
}

void check_bar()
{
  tm_quark_info qu;
  qu.bc[0]=QUARK_BOUND_COND;
  for(int mu=1;mu<NDIM;mu++) qu.bc[mu]=0;
  qu.kappa=qkappa[0];
  qu.mass=qmass[0];
  qu.r=qr[0];
  
  master_printf(" -----------------baryon direct -------------------- \n");
  
  //compute the propagator traced
  complex *mess=nissa_malloc("mess",glb_vol,complex);
  vector_reset(mess);
  
  NISSA_LOC_VOL_LOOP(r)
    {
      double dr=den_of_mom(r,qu);
      mess[r][RE]=2*qu.mass/dr/glb_vol;
      mess[r][IM]=-2*sin(mom_comp_of_site(r,qu,0))/dr/glb_vol;
    }
  
  bar_contr_free(mess,qu);
  
  nissa_free(mess);
}

void check_bar2()
{
  tm_quark_info qu;
  qu.bc[0]=QUARK_BOUND_COND;
  for(int mu=1;mu<NDIM;mu++) qu.bc[mu]=0;
  qu.kappa=qkappa[0];
  qu.mass=qmass[0];
  qu.r=qr[0];
  
  master_printf(" -----------------baryon direct ins on outdiquark -------------------- \n");
  
  //compute the propagator with self-energy insertion, traced
  complex *mess=nissa_malloc("mess",glb_vol,complex);
  vector_reset(mess);
  
  //photon prop
  spin1prop *pho_pro=nissa_malloc("pho_pro",loc_vol,spin1prop);
  NISSA_LOC_VOL_LOOP(p)
    mom_space_tlSym_gauge_propagator_of_imom(pho_pro[p],photon,p);
  //including 1/V
  
  double mu2=qu.mass*qu.mass;
  NISSA_LOC_VOL_LOOP(p)
    {
      momentum_t sin_p;
      sin_mom(sin_p,p,qu);
      double Mp=M_of_mom(qu,p);
      double pp=mom_prod(sin_p,sin_p),Mpp=Mp*Mp;
      
      NISSA_LOC_VOL_LOOP(q)
        {
	  double Mq=M_of_mom(qu,q);
	  momentum_t sin_q;
	  sin_mom(sin_q,q,qu);
	  
	  //find p-q and compute gluon prop
	  coords cpmq;
	  for(int mu=0;mu<NDIM;mu++) cpmq[mu]=(glb_size[mu]+glb_coord_of_loclx[p][mu]-glb_coord_of_loclx[q][mu])%glb_size[mu];
	  int pmq=glblx_of_coord(cpmq);
	  
	  double pq=mom_prod(sin_p,sin_q);
	  complex num={
	    8*qu.mass*(-Mpp+2*Mp*Mq+mu2-pp+pq),
	    -4*(2*sin_p[0]*(2*(Mp*Mq+mu2)+pq)-(Mpp+mu2+pp)*sin_q[0])};
	  
	  complex_summ_the_prod_double(mess[p],num,pho_pro[pmq][0][0][RE]/den_of_mom(q,qu));
	}
      complex_prodassign_double(mess[p],1/(sqr(den_of_mom(p,qu))*glb_vol));
    }
  
  bar_contr_free(mess,qu);
  
  nissa_free(mess);
  nissa_free(pho_pro);
}

double Co(int p,int q,tm_quark_info qu,int mu)
{return cos((mom_comp_of_site(p,qu,mu)+mom_comp_of_site(q,qu,mu))/2);}
double Si(int p,int q,tm_quark_info qu,int mu)
{return sin((mom_comp_of_site(p,qu,mu)+mom_comp_of_site(q,qu,mu))/2);}

void check_bar3()
{
  tm_quark_info qu;
  qu.bc[0]=QUARK_BOUND_COND;
  for(int mu=1;mu<NDIM;mu++) qu.bc[mu]=0;
  qu.kappa=qkappa[0];
  qu.mass=qmass[0];
  qu.r=qr[0];
  
  master_printf(" -----------------baryon direct cons ins on outdiquark -------------------- \n");
  
  //compute the propagator with self-energy insertion, traced
  complex *mess=nissa_malloc("mess",glb_vol,complex);
  vector_reset(mess);
  
  //photon prop
  spin1prop *pho_pro=nissa_malloc("pho_pro",loc_vol,spin1prop);
  NISSA_LOC_VOL_LOOP(p)
    mom_space_tlSym_gauge_propagator_of_imom(pho_pro[p],photon,p);
  //including 1/V
  
  double mu=qu.mass;
  double mu2=mu*mu;
  NISSA_LOC_VOL_LOOP(p)
    {
      momentum_t sin_p;
      sin_mom(sin_p,p,qu);
      double Mp=M_of_mom(qu,p);
      double pp=mom_prod(sin_p,sin_p),Mpp=Mp*Mp;
      double p0=sin_p[0];
      
      NISSA_LOC_VOL_LOOP(q)
        {
	  double Mq=M_of_mom(qu,q);
	  momentum_t sin_q;
	  sin_mom(sin_q,q,qu);
	  double q0=sin_q[0];
	  
	  //find p-q and compute gluon prop
	  coords cpmq;
	  for(int nu=0;nu<NDIM;nu++) cpmq[nu]=(glb_size[nu]+glb_coord_of_loclx[p][nu]-glb_coord_of_loclx[q][nu])%glb_size[nu];
	  int pmq=glblx_of_coord(cpmq);
	  
	  double pq=mom_prod(sin_p,sin_q);
	  complex num={0,0};
	  
	  for(int nu=0;nu<4;nu++)
	    {
	      double pnu=sin_p[nu];
	      double qnu=sin_q[nu];
	      
	      num[RE]+=
		2*mu*(4*(Mq*pnu + Mp*qnu)*Co(p,q,qu,nu)*Si(p,q,qu,nu) + 
		      (Mpp - 2*Mp*Mq - mu2 + pp - 2*pq + 4*pnu*qnu)*sqr(Co(p,q,qu,nu)) + 
		      (Mpp + 2*Mp*Mq - mu2 + pp - 2*pq)*sqr(Si(p,q,qu,nu)));
	      
	      num[IM]+=
		2*(2*Co(p,q,qu,nu)*(-2*Mp*p0*qnu + 
				    Mq*(-2*p0*pnu + (Mpp + mu2 + pp)*(nu == 0)))*Si(p,q,qu,nu) + 
		   (2*p0*(Mp*Mq + mu2 + pq - 2*pnu*qnu) - (Mpp + mu2 + pp)*(q0 - 2*qnu*(nu == 0)))*
		   sqr(Co(p,q,qu,nu)) + (2*p0*(-(Mp*Mq) + mu2 + pq) - (Mpp + mu2 + pp)*q0)*
		   sqr(Si(p,q,qu,nu)));
	    }
	  
	  complex_summ_the_prod_double(mess[p],num,pho_pro[pmq][0][0][RE]/den_of_mom(q,qu));
	}
      complex_prodassign_double(mess[p],1/(sqr(den_of_mom(p,qu))*glb_vol));
    }
  
  bar_contr_free(mess,qu);
  
  nissa_free(mess);
  nissa_free(pho_pro);
}

void mes_transf(complex *co,tm_quark_info qu)
{
  for(int p=0;p<glb_size[0];p++)
    master_printf("%d %.16lg %.16lg\n",p,co[p][0],co[p][1]);
  master_printf("\n");
  
  tm_quark_info me=qu;
  me.bc[0]=0;
  
  complex c[glb_size[0]];
  for(int x=0;x<glb_size[0];x++)
    {
      complex_put_to_zero(c[x]);
      for(int ip0=0;ip0<glb_size[0];ip0++)
	{
	  double p0=mom_comp_of_coord(ip0,me,0);
	  complex fact={cos(p0*x),sin(p0*x)};
	  complex_summ_the_prod(c[x],co[ip0],fact);
	}
      master_printf("%d %.16lg %.16lg\n",x,c[x][0],c[x][1]);
    }
}

void check_mes()
{
  tm_quark_info qu;
  qu.bc[0]=QUARK_BOUND_COND;
  for(int mu=1;mu<NDIM;mu++) qu.bc[mu]=0;
  qu.kappa=qkappa[0];
  qu.mass=qmass[0];
  qu.r=qr[0];
  
  master_printf(" ------------------ meson------------------ \n");
  
  double mu2=qu.mass*qu.mass;
  complex co[glb_size[0]];
  memset(co,0,sizeof(complex)*glb_size[0]);
  NISSA_LOC_VOL_LOOP(p)
    {
      momentum_t sin_p;
      sin_mom(sin_p,p,qu);
      double dp=den_of_mom(p,qu);
      double Mp=M_of_mom(qu,p);
      
      for(int q0=0;q0<glb_size[0];q0++)
	{
	  coords cq;
	  cq[0]=q0;
	  for(int mu=1;mu<NDIM;mu++) cq[mu]=glb_coord_of_loclx[p][mu];
	  int q=loclx_of_coord(cq);
	  
	  momentum_t sin_q;
	  sin_mom(sin_q,q,qu);
	  double dq=den_of_mom(q,qu);
	  double Mq=M_of_mom(qu,q);
	  double pq=mom_prod(sin_p,sin_q);
	
	int pmq0=(glb_size[0]+glb_coord_of_loclx[p][0]-glb_coord_of_loclx[q][0])%glb_size[0];
	double contr=4*(mu2+pq+Mp*Mq);
	
	co[pmq0][RE]+=NCOL*contr/(glb_size[0]*glb_vol*dp*dq);
      }
    }
  
  mes_transf(co,qu);
}

void check_mes2()
{
  tm_quark_info qu;
  qu.bc[0]=QUARK_BOUND_COND;
  for(int mu=1;mu<NDIM;mu++) qu.bc[mu]=0;
  qu.kappa=qkappa[0];
  qu.mass=qmass[0];
  qu.r=qr[0];
  
  master_printf(" ------------------ meson with self energy ------------------ \n");
  
  spin1prop *pho_pro=nissa_malloc("pho_pro",loc_vol,spin1prop);
  NISSA_LOC_VOL_LOOP(p)
    mom_space_tlSym_gauge_propagator_of_imom(pho_pro[p],photon,p);
  
  double mu2=qu.mass*qu.mass;
  double mu4=mu2*mu2;
  
  complex co[glb_size[0]];
  memset(co,0,sizeof(complex)*glb_size[0]);
  NISSA_LOC_VOL_LOOP(p)
    {
      momentum_t sin_p;
      sin_mom(sin_p,p,qu);
      double dp=den_of_mom(p,qu);
      double Mp=M_of_mom(qu,p);
      double Mpp=Mp*Mp;
      double pp=mom_prod(sin_p,sin_p);
      
      NISSA_LOC_VOL_LOOP(q)
        {
	  momentum_t sin_q;
	  sin_mom(sin_q,q,qu);
	  double dq=den_of_mom(q,qu);
	  double Mq=M_of_mom(qu,q);
	  double pq=mom_prod(sin_p,sin_q);
	  
	  //find p-q and compute gluon prop
	  coords cpmq;
	  for(int mu=0;mu<NDIM;mu++) cpmq[mu]=(glb_size[mu]+glb_coord_of_loclx[p][mu]-glb_coord_of_loclx[q][mu])%glb_size[mu];
	  int pmq=glblx_of_coord(cpmq);
	  
	  for(int t0=0;t0<glb_size[0];t0++)
	    {
	      coords ct;
	      ct[0]=t0;
	      for(int mu=1;mu<NDIM;mu++) ct[mu]=glb_coord_of_loclx[p][mu];
	      int t=loclx_of_coord(ct);
	      momentum_t sin_t;
	      sin_mom(sin_t,t,qu);
	      double dt=den_of_mom(t,qu);
	      double Mt=M_of_mom(qu,t);
	      double pt=mom_prod(sin_p,sin_t);
	      double qt=mom_prod(sin_q,sin_t);
	      
	      int pmt0=(glb_size[0]+glb_coord_of_loclx[p][0]-t0)%glb_size[0];
	      double num=8*(2*mu4+2*(Mpp*(Mq*Mt-mu2)-Mq*Mt*(mu2+pp)+mu2*(-pp+pq)+(2*mu2+pq)*pt+Mp*(Mt*(2*mu2+pq)+2*Mq*(mu2+pt)))-(Mpp+mu2+pp)*qt);
	      co[pmt0][RE]+=NCOL*num*pho_pro[pmq][0][0][RE]/(dp*dq*dp*dt)/glb_vol/glb_size[0];
	    }
	}
    }
  
  mes_transf(co,qu);
  
  nissa_free(pho_pro);
}

void check_mes3()
{
  tm_quark_info qu;
  qu.bc[0]=QUARK_BOUND_COND;
  for(int mu=1;mu<NDIM;mu++) qu.bc[mu]=0;
  qu.kappa=qkappa[0];
  qu.mass=qmass[0];
  qu.r=qr[0];
  
  master_printf(" ------------------ meson with self energy conservata ------------------ \n");
  
  spin1prop *pho_pro=nissa_malloc("pho_pro",loc_vol,spin1prop);
  NISSA_LOC_VOL_LOOP(p)
    mom_space_tlSym_gauge_propagator_of_imom(pho_pro[p],photon,p);
  
  double mu2=qu.mass*qu.mass;
  double mu4=mu2*mu2;
  
  complex co[glb_size[0]];
  memset(co,0,sizeof(complex)*glb_size[0]);
  NISSA_LOC_VOL_LOOP(p)
    {
      momentum_t sin_p;
      sin_mom(sin_p,p,qu);
      double dp=den_of_mom(p,qu);
      double Mp=M_of_mom(qu,p);
      double Mpp=Mp*Mp;
      double pp=mom_prod(sin_p,sin_p);
      
      NISSA_LOC_VOL_LOOP(q)
        {
	  momentum_t sin_q;
	  sin_mom(sin_q,q,qu);
	  double dq=den_of_mom(q,qu);
	  double Mq=M_of_mom(qu,q);
	  double pq=mom_prod(sin_p,sin_q);
	  
	  //find p-q and compute gluon prop
	  coords cpmq;
	  for(int mu=0;mu<NDIM;mu++) cpmq[mu]=(glb_size[mu]+glb_coord_of_loclx[p][mu]-glb_coord_of_loclx[q][mu])%glb_size[mu];
	  int pmq=glblx_of_coord(cpmq);
	  
	  for(int it0=0;it0<glb_size[0];it0++)
	    {
	      coords ct;
	      ct[0]=it0;
	      for(int mu=1;mu<NDIM;mu++) ct[mu]=glb_coord_of_loclx[p][mu];
	      int t=loclx_of_coord(ct);
	      momentum_t sin_t;
	      sin_mom(sin_t,t,qu);
	      double dt=den_of_mom(t,qu);
	      double Mt=M_of_mom(qu,t);
	      double pt=mom_prod(sin_p,sin_t);
	      double qt=mom_prod(sin_q,sin_t);
	      
	      int pmt0=(glb_size[0]+glb_coord_of_loclx[p][0]-it0)%glb_size[0];
	      double num=0;
	      for(int nu=0;nu<4;nu++)
		{
		  double p1=sin_p[nu];
		  double q1=sin_q[nu];
		  double t1=sin_t[nu];
		  num+=-4*(-2*((Mpp*Mt - Mt*(mu2 + pp) + 2*Mp*(mu2 + pt))*q1 + 
			       Mq*(2*p1*(Mp*Mt + mu2 + pt) - (Mpp + mu2 + pp)*t1))*Co(p,q,qu,nu)*Si(p,q,qu,nu) + 
			   (mu4 - mu2*pp - Mq*Mt*(mu2 + pp) + 2*mu2*pq + 2*mu2*pt + 2*pq*pt + 
			    2*Mp*(Mt*(mu2 + pq) + Mq*(mu2 + pt)) - (mu2 + pp)*qt - 
			    Mpp*(-(Mq*Mt) + mu2 + qt) + 
			    2*q1*(-2*p1*(Mp*Mt + mu2 + pt) + (Mpp + mu2 + pp)*t1))*sqr(Co(p,q,qu,nu))+
			   (mu4 - mu2*pp + Mq*Mt*(mu2 + pp) + 2*mu2*pq + 2*mu2*pt + 2*pq*pt + 
			    2*Mp*(Mt*(mu2 + pq) - Mq*(mu2 + pt)) - (mu2 + pp)*qt - Mpp*(Mq*Mt + mu2 + qt))*
			   sqr(Si(p,q,qu,nu)));
		}
	      co[pmt0][RE]+=NCOL*num*pho_pro[pmq][0][0][RE]/(dp*dq*dp*dt)/glb_vol/glb_size[0];
	    }
	}
    }
  
  mes_transf(co,qu);
  
  nissa_free(pho_pro);
}

void check_gen()
{
  double saa=0,sab=0,sbb=0,sa=0,sb=0;
  int n=1000000;
  for(int i=1;i<=n;i++)
    {
      double a=rnd_get_unif(loc_rnd_gen+0,0,1);
      double b=rnd_get_unif(loc_rnd_gen+1,0,1);
      saa+=a*a;
      sab+=a*b;
      sbb+=b*b;
      sa+=a;
      sb+=b;
    }
  //if(i%1000==0)
	{
	  double eaa=saa/n;
	  double eab=sab/n;
	  double ebb=sbb/n;
	  double ea=sa/n;
	  double eb=sb/n;
	  double c=(eab-ea*eb)/sqrt((eaa-ea*ea)*(ebb-eb*eb));
	  master_printf("%lg\n",c);
	}
    
}

//init everything
void init_simulation(char *path)
{
  //open input file and read it
  open_input(path);
  read_input_preamble();
  read_photon_pars();
}

void in_main(int narg,char **arg)
{
  //Basic mpi initialization
  tot_prog_time-=take_time();
  
  //check argument
  if(narg<2) crash("Use: %s input_file",arg[0]);
  
  //init simulation according to input file
  init_simulation(arg[1]);
  
  check_mes();
  master_printf("\n\n");
  check_mes2();
  master_printf("\n\n");
  check_mes3();
  master_printf("\n\n");
  check_bar();
  master_printf("\n\n");
  check_bar2();
  master_printf("\n\n");
  check_bar3();
  master_printf("\n\n");
  for(int i=0;i<2000;i++) check_gen();
  MPI_Finalize();
  exit(0);
}

int main(int narg,char **arg)
{
  init_nissa_threaded(narg,arg,in_main);
  close_nissa();
  
  return 0;
}