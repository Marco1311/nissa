#include <nissa.hpp>

#include "conf.hpp"
#include "pars.hpp"

#ifdef POINT_SOURCE_VERSION
 #define PROP_TYPE su3spinspin
#else
 #define PROP_TYPE colorspinspin
#endif

using namespace nissa;

/////////////////////////////////////// data //////////////////////////////

int loc_pion_curr;
int loc_muon_curr;

int chris_test;
int chris_t;

int ninv_tot=0,nhadr_contr_tot=0,nlept_contr_tot=0,nsource_tot=0,nphoton_prop_tot=0;
double inv_time=0,hadr_contr_time=0,lept_contr_time=0,print_time=0;
double tot_prog_time=0,source_time=0,photon_prop_time=0,lepton_prop_time=0;

int wall_time;
int follow_chris_or_nazario;
int free_theory,rnd_gauge_transform;
int ngauge_conf,nanalyzed_conf=0;
char conf_path[1024],outfolder[1024];
quad_su3 *conf;

tm_basis_t base;
int pure_wilson;
double kappa;
double put_theta[4],old_theta[4]={0,0,0,0};

PROP_TYPE *source,*original_source;
int seed,noise_type;

int nqmass,nr,nsources;
double *qmass,*qkappa,*residue;
PROP_TYPE **Q;

spincolor *temp_source;
spincolor *temp_solution;

gauge_info photon;
double tadpole[4];
spin1field *photon_field[nphi_eta_alt];

int hadr_corr_length;
complex *hadr_corr;
const int nhadr_contr=16;
const int ig_hadr_so[nhadr_contr]={ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
const int ig_hadr_si[nhadr_contr]={ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15};
complex *glb_corr,*loc_corr;

//list the 8 matrices to insert for the weak current
const int nweak_ins=16;
const int nweak_ind=8;
//const int nhadrolept_proj=4,hadrolept_projs[nhadrolept_proj]={9,4,5,0};
const int nhadrolept_proj=1,hadrolept_projs[nhadrolept_proj]={4};
const int list_weak_insq[nweak_ins]=     {1,2,3,4, 6,7,8,9,  1,2,3,4, 6,7,8,9};
const int list_weak_insl[nweak_ins]=     {1,2,3,4, 6,7,8,9,  6,7,8,9, 1,2,3,4};
const int list_weak_ind_contr[nweak_ins]={0,0,0,1, 2,2,2,3,  4,4,4,5, 6,6,6,7};
const char list_weak_ind_nameq[nweak_ind][3]={"VK","V0","AK","A0","VK","V0","AK","A0"};
const char list_weak_ind_namel[nweak_ind][3]={"VK","V0","AK","A0","AK","A0","VK","V0"};
int nind;
spinspin *hadr;
complex *hadrolept_corr_chris;
complex *hadrolept_corr;

//hadron contractions
const int ncombo_hadr_corr=9;
const qprop_t prop1_hadr_map[ncombo_hadr_corr]={PROP_0,PROP_0,PROP_0,PROP_0,     PROP_0,PROP_PHI,PROP_ALT,PROP_0};
const qprop_t prop2_hadr_map[ncombo_hadr_corr]={PROP_0,PROP_S,PROP_P,PROP_PHIETA,PROP_T,PROP_ETA,PROP_ALT,PROP_ALT_ALT};

//parameters of the leptons
int nleptons;
int *lep_corr_iq1;
int *lep_corr_iq2;
tm_quark_info *leps;
double *lep_energy,*neu_energy;
spinspin **L,*temp_lep;

//prototype
void generate_original_source();
void generate_photon_stochastic_propagator();

////////////////////////////////////////////// get propagator and prop. info /////////////////////////////////////////////

//return appropriate propagator
int nqprop,nlprop;
int iqprop(int imass,int ip,int r)
{return r+nr*(imass+nqmass*ip);}
int ilprop(int ilepton,int orie,int phi_eta_alt,int r,int t2)
{return r+nr*(phi_eta_alt+nphi_eta_alt*(orie+norie*(ilepton+nleptons*t2)));}

//return appropriately modified info
tm_quark_info get_lepton_info(int ilepton,int orie,int r)
{
  tm_quark_info le=leps[ilepton];
  le.r=r;
  for(int i=1;i<NDIM;i++) le.bc[i]*=sign_orie[orie];
  
  return le;
}

///////////////////////////////// initialise the library, read input file, allocate /////////////////////////////////////

void init_simulation(char *path)
{
  //open input file
  open_input(path);
  
  //init the grid
  {
    int L,T;
    read_str_int("L",&L);
    read_str_int("T",&T);
    //Init the MPI grid
    init_grid(T,L);
  }
  
  //Wall time
  read_str_int("WallTime",&wall_time);
  //Pure Wilson
  read_str_int("PureWilson",&pure_wilson);
  if(pure_wilson)
    {
      base=WILSON_BASE;
      nr=1;
      read_list_of_double_pairs("QKappaResidues",&nqmass,&qkappa,&residue);
    }
  else
    {
      base=MAX_TWIST_BASE;
      //Kappa
      read_str_double("Kappa",&kappa);
      //One or two r
      read_str_int("NR",&nr);
      //Masses and residue
      read_list_of_double_pairs("QMassResidues",&nqmass,&qmass,&residue);
    }
  
  //Leptons
  read_str_int("LeptonicCorrs",&nleptons);
  lep_corr_iq1=nissa_malloc("lep_corr_iq1",nleptons,int);
  lep_corr_iq2=nissa_malloc("lep_corr_iq2",nleptons,int);
  leps=nissa_malloc("leps",nleptons,tm_quark_info);
  lep_energy=nissa_malloc("lep_energy",nleptons,double);
  neu_energy=nissa_malloc("neu_energy",nleptons,double);
  if(!pure_wilson) expect_str("Q1Q2LepmassMesmass");
  else             expect_str("Q1Q2LepkappaMesmass");
  for(int il=0;il<nleptons;il++)
    {
      //read quarks identfiying the mesons
      read_int(lep_corr_iq1+il);
      read_int(lep_corr_iq2+il);
      
      //if not pure wilson read mass
      if(pure_wilson) leps[il].mass=0;
      else            read_double(&leps[il].mass);
      
      //antiperiodic
      leps[il].bc[0]=1;
      
      //maximal twist (if tm), otherwise read kappa
      if(pure_wilson) read_double(&leps[il].kappa);
      else            leps[il].kappa=0.125;
      leps[il].r=0;
      
      //read the mass of the meson (that must have been determined outside)
      double mes_mass;
      read_double(&mes_mass);
      
      //set initial value of bc and check kinematic
      for(int i=1;i<NDIM;i++) leps[il].bc[i]=0;
      if(tm_quark_energy(leps[il],0)>=mes_mass) crash("initial state is lighter (%lg) than final state at rest (%lg)!",mes_mass,tm_quark_energy(leps[il],0));
      
      //compute meson momentum and bc
      double err;
      do
      	{
      	  //compute the error
	  double lep_energy=tm_quark_energy(leps[il],0);
	  double neu_energy=naive_massless_quark_energy(leps[il].bc,0);
      	  err=lep_energy+neu_energy-mes_mass;
      	  //compute the derivative
      	  double eps=1e-8;
      	  for(int i=1;i<NDIM;i++) leps[il].bc[i]+=eps;
      	  double der=(tm_quark_energy(leps[il],0)+naive_massless_quark_energy(leps[il].bc,0)-mes_mass-err)/eps;
      	  for(int i=1;i<NDIM;i++) leps[il].bc[i]-=eps+err/der;
	  
      	  master_printf("lep_e: %+010.10lg, neu_e: %+010.10lg, mes_mass: %lg, error: %lg, der: %lg\n",lep_energy,neu_energy,mes_mass,err,der);
      	}
      while(fabs(err)>1e-14);
      
      //write down energy
      lep_energy[il]=tm_quark_energy(leps[il],0);
      neu_energy[il]=naive_massless_quark_energy(leps[il].bc,0);
      master_printf(" ilepton %d, lepton energy: %lg, neutrino energy: %lg\n",il,lep_energy[il],neu_energy[il]);
      master_printf(" lep+neut energy: %lg\n",lep_energy[il]+neu_energy[il]);
      master_printf(" bc: %+016.016lg\n",leps[il].bc[1]);
    }
  
  //Zero mode subtraction
  char zero_mode_sub_str[100];
  read_str_str("ZeroModeSubtraction",zero_mode_sub_str,100);
  
  if(strncasecmp(zero_mode_sub_str,"PECIONA",100)==0) photon.zms=PECIONA;
  else
    if(strncasecmp(zero_mode_sub_str,"UNNO_ALEMANNA",100)==0) photon.zms=UNNO_ALEMANNA;
    else
      if(strncasecmp(zero_mode_sub_str,"ONLY_100",100)==0) photon.zms=ONLY_100;
      else crash("Unkwnown zero mode subtraction: %s",zero_mode_sub_str);
  
  //gauge for photon propagator
  char photon_gauge_str[100];
  read_str_str("PhotonGauge",photon_gauge_str,100);
  if(strncasecmp(photon_gauge_str,"FEYNMAN",100)==0) photon.alpha=FEYNMAN_ALPHA;
  else
    if(strncasecmp(photon_gauge_str,"LANDAU",100)==0) photon.alpha=LANDAU_ALPHA;
    else
      if(strncasecmp(photon_gauge_str,"LANDAU",100)==0) read_str_double("Alpha",&photon.alpha);
      else crash("Unkwnown photon gauge: %s",photon_gauge_str);
  
  //discretization for photon propagator
  char photon_discrete_str[100];
  read_str_str("PhotonDiscretization",photon_discrete_str,100);
  if(strncasecmp(photon_discrete_str,"WILSON",100)==0) photon.c1=WILSON_C1;
  else
    if(strncasecmp(photon_discrete_str,"TLSYM",100)==0) photon.c1=TLSYM_C1;
    else crash("Unkwnown photon discretization: %s",photon_discrete_str);
  
  //initialize the random generator with the read seed
  read_str_int("Seed",&seed);
  start_loc_rnd_gen(seed);
  //noise type
  read_str_int("NoiseType",&noise_type);
  
  //flag to simulate in the free theory
  read_str_int("FreeTheory",&free_theory);
  
  //flag to make the muon with or without the external line
  read_str_int("FollowChrisOrNazario",&follow_chris_or_nazario);
  
  //perform a random gauge transformation
  read_str_int("RandomGaugeTransform",&rnd_gauge_transform);
  
  //make chris test?
  read_str_int("ChrisTest",&chris_test);
  if(chris_test) read_str_int("ChrisT",&chris_t);
  
  //local current on muon or pion
  read_str_int("LocPionCurr",&loc_pion_curr);
  read_str_int("LocMuonCurr",&loc_muon_curr);
  
  //number of sources
  read_str_int("NSources",&nsources);
  
  //number of configurations
  read_str_int("NGaugeConf",&ngauge_conf);
  
  ///////////////////// finihed reading apart from conf list ///////////////
  
  //compute the tadpole summing all momentum
  compute_tadpole(tadpole,photon);
  
  //Allocate
  nqprop=iqprop(nqmass-1,nqprop_kind-1,nr-1)+1;
  nlprop=ilprop(nleptons-1,norie-1,nphi_eta_alt-1,nr-1,glb_size[0])+1; //added for chris
  
  //allocate temporary vectors
  temp_source=nissa_malloc("temp_source",loc_vol,spincolor);
  temp_solution=nissa_malloc("temp_solution",loc_vol,spincolor);
  hadr_corr_length=glb_size[0]*nhadr_contr*ncombo_hadr_corr*nqmass*nqmass*nr;
  hadr_corr=nissa_malloc("hadr_corr",hadr_corr_length,complex);
  glb_corr=nissa_malloc("glb_corr",glb_size[0]*nhadr_contr,complex);
  loc_corr=nissa_malloc("loc_corr",glb_size[0]*nhadr_contr,complex);
  nind=nleptons*nweak_ind*nphi_eta_alt*norie*nr;
  hadr=nissa_malloc("hadr",loc_vol,spinspin);
  hadrolept_corr=nissa_malloc("hadrolept_corr",glb_size[0]*nweak_ind*nhadrolept_proj*nind,complex);
  hadrolept_corr_chris=nissa_malloc("hadrolept_corr_chris",glb_size[0]*glb_size[0]*nweak_ind*nhadrolept_proj*nind,complex);
  original_source=nissa_malloc("source",loc_vol,PROP_TYPE);
  source=nissa_malloc("source",loc_vol,PROP_TYPE);
  for(int i=0;i<nphi_eta_alt;i++) photon_field[i]=nissa_malloc("photon_phield",loc_vol+bord_vol,spin1field);
  Q=nissa_malloc("Q*",nqprop,PROP_TYPE*);
  for(int iprop=0;iprop<nqprop;iprop++) Q[iprop]=nissa_malloc("Q",loc_vol+bord_vol,PROP_TYPE);
  L=nissa_malloc("L*",nlprop,spinspin*);
  for(int iprop=0;iprop<nlprop;iprop++) L[iprop]=nissa_malloc("L",loc_vol+bord_vol,spinspin);
  temp_lep=nissa_malloc("temp_lep",loc_vol+bord_vol,spinspin);
  conf=nissa_malloc("conf",loc_vol+bord_vol,quad_su3);
}

//find a new conf
int read_conf_parameters(int &iconf)
{
  int ok_conf;
  
  do
    {
      //Gauge path
      read_str(conf_path,1024);
      
      //Out folder
      read_str(outfolder,1024);
      
      //Check if the conf has been finished or is already running
      master_printf("Considering configuration \"%s\" with output path \"%s\".\n",conf_path,outfolder);
      char fin_file[1024],run_file[1024];
      sprintf(fin_file,"%s/finished",outfolder);
      sprintf(run_file,"%s/running",outfolder);
      ok_conf=!(file_exists(fin_file)) && !(file_exists(run_file));
      
      //if not finished
      if(ok_conf)
	{
	  master_printf(" Configuration \"%s\" not yet analyzed, starting",conf_path);
	  if(!dir_exists(outfolder))
	    {
	      int ris=create_dir(outfolder);
	      if(ris==0) master_printf(" Output path \"%s\" not present, created.\n",outfolder);
	      else
		crash(" Failed to create the output \"%s\" for conf \"%s\".\n",outfolder,conf_path);
	    }
	  file_touch(run_file);
	}
      else
	{
	  master_printf(" In output path \"%s\" terminating file already present: configuration \"%s\" already analyzed, skipping.\n",outfolder,conf_path);
	  for(int isource=0;isource<nsources;isource++)
	    {
	      coords coord;
	      generate_random_coord(coord);
	      generate_stochastic_tlSym_gauge_propagator_source(photon_field[ieta]);
	      generate_original_source();
	    }
	}
      iconf++;
    }
  while(!ok_conf && iconf<ngauge_conf);
  
  master_printf("\n");
  
  return ok_conf;
}

//init a new conf
void start_new_conf()
{
  setup_conf(conf,old_theta,put_theta,conf_path,rnd_gauge_transform,free_theory);
  
  //reset correlations
  vector_reset(hadr_corr);
  vector_reset(hadrolept_corr);
  vector_reset(hadrolept_corr_chris);
}

//generate a wall-source for stochastic QCD propagator
void generate_original_source()
{
  //reset the real source position
  coords origin_coord;
  for(int mu=0;mu<NDIM;mu++) origin_coord[mu]=0;
  
#ifdef POINT_SOURCE_VERSION
  master_printf("Source position: t=%d x=%d y=%d z=%d\n",origin_coord[0],origin_coord[1],origin_coord[2],origin_coord[3]);
  generate_delta_source(original_source,origin_coord);
#else
  generate_spindiluted_source(original_source,rnd_type_map[noise_type],origin_coord[0]);
#endif
}

//////////////////////////////////////// quark propagators /////////////////////////////////////////////////

//insert the photon on the source side
void insert_external_loc_source(PROP_TYPE *out,spin1field *curr,coords dirs,PROP_TYPE *in,int t)
{ 
  GET_THREAD_ID();
  
  if(in==out) crash("in==out");
  
  vector_reset(out);
  
  for(int mu=0;mu<NDIM;mu++)
    if(dirs[mu])
      NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	if(t==-1||glb_coord_of_loclx[ivol][0]==t)
	  {
	    PROP_TYPE temp1,temp2;
	    NAME2(unsafe_dirac_prod,PROP_TYPE)(temp1,base_gamma+map_mu[mu],in[ivol]);
	    NAME3(unsafe,PROP_TYPE,prod_complex)(temp2,temp1,curr[ivol][mu]);
	    NAME2(PROP_TYPE,summ_the_prod_idouble)(out[ivol],temp2,1);
	  }
  
  set_borders_invalid(out);
}
//insert the photon on the source
void insert_external_loc_source(PROP_TYPE *out,spin1field *curr,PROP_TYPE *in,int t)
{insert_external_loc_source(out,curr,all_dirs,in,t);}

void insert_external_source(PROP_TYPE *out,spin1field *curr,PROP_TYPE *ori,int t,int r,int loc)
{
  if(loc) insert_external_loc_source(source,photon_field[iphi],ori,t);
  else
    if(!pure_wilson) insert_tm_external_source(source,conf,photon_field[iphi],ori,r,t);
    else             insert_wilson_external_source(source,conf,photon_field[iphi],ori,t);
}

//generate a sequential source
void generate_source(insertion_t inser,int r,PROP_TYPE *ori,int t=-1)
{
  source_time-=take_time();
  
  if(t!=-1 && inser!=STOCH_PHI) crash("not valid");
  
  //double ori_norm;
  //double_vector_glb_scalar_prod(&ori_norm,(double*)ori,(double*)ori,sizeof(PROP_TYPE)/sizeof(double)*loc_vol);
  //master_printf("ori_norm2: %lg\n",ori_norm);
  
  switch(inser)
    {
    case ORIGINAL:prop_multiply_with_gamma(source,0,original_source);break;
    case SCALAR:prop_multiply_with_gamma(source,0,ori);break;
    case PSEUDO:prop_multiply_with_gamma(source,5,ori);break;
    case STOCH_PHI:insert_external_source(source,photon_field[iphi],ori,t,r,loc_pion_curr);break;
    case STOCH_ETA:insert_external_source(source,photon_field[ieta],ori,t,r,loc_pion_curr);break;
    case STOCH_ALT:insert_external_source(source,photon_field[ialt],ori,t,r,loc_pion_curr);break;
    case TADPOLE:
      if(!pure_wilson) insert_tm_tadpole(source,conf,ori,r,tadpole,-1);
      else             insert_wilson_tadpole(source,conf,ori,tadpole,-1);
      break;
    }
  
  //double after_norm;
  //double_vector_glb_scalar_prod(&after_norm,(double*)source,(double*)source,sizeof(PROP_TYPE)/sizeof(double)*loc_vol);
  //master_printf("after_norm2: %lg\n",after_norm);
  
  source_time+=take_time();
  nsource_tot++;
}

//invert on top of a source, putting all needed for the appropriate quark
void get_qprop(PROP_TYPE *out,PROP_TYPE *in,int imass,bool r)
{
  //these are the ways in which Dirac operator rotates - propagator is opposite, see below
#ifdef POINT_SOURCE_VERSION
  for(int ic=0;ic<NCOL;ic++)
#endif
    for(int id=0;id<4;id++)
      { 
	//read the source out
#ifdef POINT_SOURCE_VERSION
	get_spincolor_from_su3spinspin(temp_source,in,id,ic);
#else
	get_spincolor_from_colorspinspin(temp_source,in,id);
#endif
	
	//rotate the source index - the propagator rotate AS the sign of mass term
	if(!pure_wilson) safe_dirac_prod_spincolor(temp_source,(tau3[r]==-1)?&Pminus:&Pplus,temp_source);
	
	//invert
	inv_time-=take_time();
	if(!pure_wilson) inv_tmD_cg_eoprec_eos(temp_solution,NULL,conf,kappa,tau3[r]*qmass[imass],100000,residue[imass],temp_source);
	else             inv_tmD_cg_eoprec_eos(temp_solution,NULL,conf,qkappa[imass],0,100000,residue[imass],temp_source);
	ninv_tot++;inv_time+=take_time();
	
	//rotate the sink index
	if(!pure_wilson) safe_dirac_prod_spincolor(temp_solution,(tau3[r]==-1)?&Pminus:&Pplus,temp_solution);
	
	//put the output on place
#ifdef POINT_SOURCE_VERSION
	master_printf("  finished the inversion dirac index %d, color %d\n",id,ic);
	put_spincolor_into_su3spinspin(out,temp_solution,id,ic);
#else
	master_printf("  finished the inversion dirac index %d\n",id);
	put_spincolor_into_colorspinspin(out,temp_solution,id);
#endif
      }
}

//generate all the quark propagators
void generate_quark_propagators()
{
  for(int ip=0;ip<nqprop_kind;ip++)
    {
      master_printf("Generating propagtor of type %s inserting %s on source %s\n",prop_name[prop_map[ip]],ins_name[insertion_map[ip]],prop_name[source_map[ip]]);
      for(int imass=0;imass<nqmass;imass++)
	for(int r=0;r<nr;r++)
	  {
	    if(!pure_wilson) master_printf(" mass[%d]=%lg, r=%d\n",imass,qmass[imass],r);
	    else             master_printf(" kappa[%d]=%lg\n",imass,qkappa[imass]);
	    generate_source(insertion_map[ip],r,Q[iqprop(imass,source_map[ip],r)]);
	    get_qprop(Q[iqprop(imass,prop_map[ip],r)],source,imass,r);
	  }
    }
}

void generate_quark_propagators_chris(int t1)
{
  if(nr==2) crash("this is chris test!");
  generate_source(STOCH_PHI,0,Q[iqprop(0,PROP_0,0)],t1);
  get_qprop(Q[iqprop(0,PROP_PHI,0)],source,0,0);
}

/////////////////////////////////////////////// photon propagators ///////////////////////////////////////////

//wrapper to generate a stochastic propagator
void generate_photon_stochastic_propagator()
{
  photon_prop_time-=take_time();
  generate_stochastic_tlSym_gauge_propagator(photon_field[iphi],photon_field[ieta],photon);
  
  //do also the alternative version
  multiply_by_sqrt_tlSym_gauge_propagator(photon_field[ialt],photon_field[ieta],photon);
  
  //TEST TEST TEST for NAZARIO
  //GET_THREAD_ID();
  // for(int ip=0;ip<3;ip++)
  //   {
  //     vector_reset(photon_field[ip]);
  //     NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
  // 	for(int mu=0;mu<NDIM;mu++)
  // 	  photon_field[ip][ivol][mu][RE]=1;
  //     set_borders_invalid(photon_field[ip]);
  //   }
  
  photon_prop_time+=take_time();
  nphoton_prop_tot++;
}

void test_photon_propagator()
{
  GET_THREAD_ID();
  
  double phi_000[glb_size[0]];
  double eta_000[glb_size[0]];
  complex phi_100[glb_size[0]];
  complex eta_100[glb_size[0]];
  double phi_200[glb_size[0]];
  double eta_200[glb_size[0]];
  
  complex ep100[glb_size[0]];
  for(int dt=0;dt<glb_size[0];dt++) complex_put_to_zero(ep100[dt]);
  
  double f=0;
  
  double p100_mom_loc[glb_size[0]];
  memset(p100_mom_loc,0,sizeof(double)*glb_size[0]);
  for(int loc_t=0;loc_t<loc_size[0];loc_t++)
    {
      int glb_t=loc_t+loc_size[0]*rank_coord[0];
      coords glb_mom_c;
      glb_mom_c[0]=glb_t;
      glb_mom_c[1]=1;
      glb_mom_c[2]=0;
      glb_mom_c[3]=0;
      
      int loc_imom,rank_of_imom;
      get_loclx_and_rank_of_coord(&loc_imom,&rank_of_imom,glb_mom_c);
      if(rank==rank_of_imom)
	{
	  spin1prop pr;
	  mom_space_tlSym_gauge_propagator_of_imom(pr,photon,loc_imom);
	  p100_mom_loc[glb_t]=pr[0][0][RE];
	}
    }
  THREAD_BARRIER();
  
  double p100_mom[glb_size[0]];
  for(int glb_t=0;glb_t<glb_size[0];glb_t++) p100_mom[glb_t]=glb_reduce_double(p100_mom_loc[glb_t]);
  
  master_printf("exact 100:\n");
  for(int t=0;t<glb_size[0];t++)
    {
      double p100_time=0;
      for(int q0=0;q0<glb_size[0];q0++) p100_time+=cos(t*q0*2*M_PI/glb_size[0])*p100_mom[q0];
      master_printf("%d %lg\n",t,p100_time);
    }
  
  //now do it stocastically
  int nhits=10;
  generate_stochastic_tlSym_gauge_propagator(photon_field[iphi],photon_field[ieta],photon);
  for(int ihit=0;ihit<nhits;ihit++)
    {
      generate_stochastic_tlSym_gauge_propagator(photon_field[iphi],photon_field[ieta],photon);
      
      memset(phi_000,0,sizeof(double)*glb_size[0]);
      memset(eta_000,0,sizeof(double)*glb_size[0]);
      memset(phi_100,0,sizeof(complex)*glb_size[0]);
      memset(eta_100,0,sizeof(complex)*glb_size[0]);
      memset(phi_200,0,sizeof(double)*glb_size[0]);
      memset(eta_200,0,sizeof(double)*glb_size[0]);
      
      NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	{
	  int t=glb_coord_of_loclx[ivol][0];
	  double c000=1;
	  double c100=cos(1*glb_coord_of_loclx[ivol][1]*2*M_PI/glb_size[1]);
	  double s100=sin(1*glb_coord_of_loclx[ivol][1]*2*M_PI/glb_size[1]);
	  complex cs100={c100,s100};
	  double c200=cos(2*glb_coord_of_loclx[ivol][1]*2*M_PI/glb_size[1]);
	  phi_000[t]+=photon_field[iphi][ivol][0][RE]*c000;
	  eta_000[t]+=photon_field[ieta][ivol][0][RE]*c000;
	  complex_summ_the_conj2_prod(phi_100[t],photon_field[iphi][ivol][0],cs100);
	  complex_summ_the_prod(eta_100[t],photon_field[ieta][ivol][0],cs100);
	  phi_200[t]+=photon_field[iphi][ivol][0][RE]*c200;
	  eta_200[t]+=photon_field[ieta][ivol][0][RE]*c200;
	  for(int jvol=0;jvol<loc_vol;jvol++) f=photon_field[ieta][ivol][0][RE];//*photon_field[ieta][jvol][0][RE];
	  //master_printf("********** %lg %lg\n",photon_field[iphi][ivol][0][IM],photon_field[ieta][ivol][0][IM]);
	}
      THREAD_BARRIER();
      
      for(int t=0;t<glb_size[0];t++)
	{
	  phi_000[t]=glb_reduce_double(phi_000[t]);
	  eta_000[t]=glb_reduce_double(eta_000[t]);
	  glb_reduce_complex(phi_100[t],phi_100[t]);
	  glb_reduce_complex(eta_100[t],eta_100[t]);
	  phi_200[t]=glb_reduce_double(phi_200[t]);
	  eta_200[t]=glb_reduce_double(eta_200[t]);
	}
      
      //take time convolution
      for(int dt=0;dt<glb_size[0];dt++)
	for(int t1=0;t1<glb_size[0];t1++)
	  {
	    int t2=(t1+dt)%glb_size[0];
	    complex_summ_the_prod(ep100[dt],phi_100[t1],eta_100[t2]);
	  }
    }
  
  for(int dt=0;dt<glb_size[0];dt++) complex_prodassign_double(ep100[dt],1.0/glb_spat_vol/nhits/glb_vol);
  
  master_printf("\n############### check photon ############\n");
  for(int t=0;t<glb_size[0];t++)
    printf("%d\t(p,e)_000=(%lg,%lg)\tep_100=(%lg,%lg)\t(p,e)_200=(%lg,%lg)\n",t,phi_000[t],eta_000[t],ep100[t][RE],ep100[t][IM],phi_200[t],eta_200[t]);
  
  //f=glb_reduce_double(f)/nhits/glb_vol;
  master_printf("%lg\n",f);
  master_printf("#########################################\n\n");
}

THREADABLE_FUNCTION_1ARG(test_muon_propagator, int,imom)
{
  spinspin *prop=nissa_malloc("prop",loc_vol+bord_vol,spinspin);
  
  tm_quark_info le=get_lepton_info(0,0,0);
  compute_x_space_twisted_propagator_by_fft(prop,le,MAX_TWIST_BASE);
  
  //get the projectors
  spinspin promu;
  //twisted_on_shell_operator_of_imom(promu,le,imom,true,+1,MAX_TWIST_BASE);
  twisted_particle_projector_of_imom(promu,le,imom,MAX_TWIST_BASE);
  
  complex corr[glb_size[0]];
  memset(corr,0,glb_size[0]*sizeof(complex));
  
  coords co;
  glb_coord_of_glblx(co,imom);
  
  GET_THREAD_ID();
  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
    {
      int glb_t=glb_coord_of_loclx[ivol][0];
      
      complex c;
      trace_spinspin_prod_spinspin(c,promu,prop[ivol]);
      double arg=
	0+
	M_PI*(2*co[1]+le.bc[1])*glb_coord_of_loclx[ivol][1]/glb_size[1]+
	M_PI*(2*co[2]+le.bc[2])*glb_coord_of_loclx[ivol][2]/glb_size[2]+
	M_PI*(2*co[3]+le.bc[3])*glb_coord_of_loclx[ivol][3]/glb_size[3];
      complex w={cos(arg),sin(-arg)};
      complex_summ_the_prod(corr[glb_t],c,w);
    }
  THREAD_BARRIER();
  
  master_printf("muon propagator (%d,%d,%d)\n",co[1],co[2],co[3]);
  for(int t=0;t<glb_size[0];t++)
    {
      corr[t][RE]=glb_reduce_double(corr[t][RE]);
      corr[t][IM]=glb_reduce_double(corr[t][IM]);
    }
  
  master_printf("Expected fall: %lg\n",tm_quark_energy(le,imom));
  for(int t=1;t<glb_size[0]-1;t++)
    master_printf(" %d %lg\n",t,log(corr[t][RE]/corr[t+1][RE]));
  
  nissa_free(prop);
}
THREADABLE_FUNCTION_END

/////////////////////////////////////////////// lepton propagators ///////////////////////////////////////////

//compute phase exponent for space part: vec{p}*\vec{x}
double get_space_arg(int ivol,momentum_t bc)
{
  double arg=0;
  for(int mu=1;mu<NDIM;mu++)
    {
      double step=bc[mu]*M_PI/glb_size[mu];
      arg+=step*glb_coord_of_loclx[ivol][mu];
    }
  return arg;
}

//compute the phase for lepton on its sink
void get_lepton_sink_phase_factor(complex out,int ivol,int ilepton,tm_quark_info le)
{
  //compute space and time factor
  double arg=get_space_arg(ivol,le.bc);
  int t=glb_coord_of_loclx[ivol][0];
  if(follow_chris_or_nazario==follow_nazario && t>=glb_size[0]/2) t=glb_size[0]-t;
  if(follow_chris_or_nazario==follow_nazario && t>=glb_size[0]/2) t=glb_size[0]-t;
  double ext=exp(t*lep_energy[ilepton]);
  
  //compute full exponential (notice the factor -1)
  out[RE]=cos(-arg)*ext;
  out[IM]=sin(-arg)*ext;
}

//compute the phase for antineutrino - the orientation is that of the muon (as above)
void get_antineutrino_source_phase_factor(complex out,int ivol,int ilepton,momentum_t bc)
{
  //compute space and time factor
  double arg=get_space_arg(ivol,bc);
  int t=glb_coord_of_loclx[ivol][0];
  if(follow_chris_or_nazario==follow_nazario && t>=glb_size[0]/2) t=glb_size[0]-t;
  double ext=exp(t*neu_energy[ilepton]);
  
  //compute full exponential (notice the factor +1)
  out[RE]=cos(+arg)*ext;
  out[IM]=sin(+arg)*ext;
}

//set everything to a phase factor
void set_to_lepton_sink_phase_factor(spinspin *prop,int ilepton,tm_quark_info &le)
{
  GET_THREAD_ID();
  
  vector_reset(prop);
  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
    {
      complex ph;
      get_lepton_sink_phase_factor(ph,ivol,ilepton,le);
      spinspin_put_to_diag(prop[ivol],ph);
    }
  set_borders_invalid(prop);
}

//insert the photon on the source side
THREADABLE_FUNCTION_5ARG(insert_photon_on_the_source, spinspin*,prop, spin1field*,A, int*,dirs, tm_quark_info,le, int,twall)
{
  GET_THREAD_ID();
  
  //select A
  communicate_lx_spin1field_borders(A);
  
  //copy on the temporary and communicate borders
  vector_copy(temp_lep,prop);
  communicate_lx_spinspin_borders(temp_lep);
  vector_reset(prop);
  
  if(!loc_muon_curr)
    {
      dirac_matr GAMMA;
      if(pure_wilson) dirac_prod_double(&GAMMA,base_gamma+0,1);
      else dirac_prod_idouble(&GAMMA,base_gamma+5,-tau3[le.r]);
      
      //phases
      quad_u1 phases;
      for(int mu=0;mu<NDIM;mu++)
	{
	  phases[mu][0]=cos(le.bc[mu]*M_PI);
	  phases[mu][1]=sin(le.bc[mu]*M_PI);
	}
      
      //prepare each propagator for a single lepton
      //by computing i(phi(x-mu)A_mu(x-mu)(-i t3 g5-gmu)/2-phi(x+mu)A_mu(x)(-i t3 g5+gmu)/2)=
      //(ph0 A_mu(x-mu)g[r][0][mu]-ph0 A_mu(x)g[r][1][mu])=
      for(int mu=0;mu<NDIM;mu++)
	if(dirs[mu])
	  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	    if(twall==-1||glb_coord_of_loclx[ivol][0]==twall)
	      {
		//find neighbors
		int ifw=loclx_neighup[ivol][mu];
		int ibw=loclx_neighdw[ivol][mu];
		
		//compute phase factor
		spinspin ph_bw,ph_fw;
		
		//transport down and up
		if(glb_coord_of_loclx[ivol][mu]==glb_size[mu]-1) unsafe_spinspin_prod_complex_conj2(ph_fw,temp_lep[ifw],phases[mu]);
		else spinspin_copy(ph_fw,temp_lep[ifw]);
		if(glb_coord_of_loclx[ivol][mu]==0) unsafe_spinspin_prod_complex(ph_bw,temp_lep[ibw],phases[mu]);
		else spinspin_copy(ph_bw,temp_lep[ibw]);
		
		//fix coefficients - i is inserted here!
		//also dir selection is made here
		spinspin_prodassign_idouble(ph_fw,-0.5*dirs[mu]);
		spinspin_prodassign_idouble(ph_bw,+0.5*dirs[mu]);
		
		//fix insertion of the current
		safe_spinspin_prod_complex(ph_fw,ph_fw,A[ivol][mu]);
		safe_spinspin_prod_complex(ph_bw,ph_bw,A[ibw][mu]);
		
		//summ and subtract the two
		spinspin fw_M_bw,fw_P_bw;
		spinspin_subt(fw_M_bw,ph_fw,ph_bw);
		spinspin_summ(fw_P_bw,ph_fw,ph_bw);
		
		//put GAMMA on the summ
		spinspin temp_P;
		unsafe_spinspin_prod_dirac(temp_P,fw_P_bw,&GAMMA);
		spinspin_summassign(prop[ivol],temp_P);
		
		//put gmu on the diff
		spinspin temp_M;
		unsafe_spinspin_prod_dirac(temp_M,fw_M_bw,base_gamma+map_mu[mu]);
		spinspin_summassign(prop[ivol],temp_M);
	      }
    }
  else
    {
      for(int mu=0;mu<NDIM;mu++)
	if(dirs[mu])
	  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	    if(twall==-1||glb_coord_of_loclx[ivol][0]==twall)
	      {
		spinspin temp1,temp2;
		unsafe_spinspin_prod_dirac(temp1,temp_lep[ivol],base_gamma+map_mu[mu]);
		unsafe_spinspin_prod_complex(temp2,temp1,A[ivol][mu]);
		spinspin_summ_the_prod_idouble(prop[ivol],temp2,1);
	      }
      
    }
  
  set_borders_invalid(prop);
}
THREADABLE_FUNCTION_END

void insert_photon_on_the_source(spinspin *prop,int iphi_eta_alt,int *dirs,tm_quark_info le,int twall)
{
  if(!loc_muon_curr) master_printf("Inserting photon [%s] point-split on time %d\n",photon_field_name[iphi_eta_alt],twall);
  else master_printf("Inserting photon [%s] locally on time %d\n",photon_field_name[iphi_eta_alt]);
  insert_photon_on_the_source(prop,photon_field[iphi_eta_alt],dirs,le,twall);
}


//insert the photon on the source
void insert_photon_on_the_source(spinspin *prop,int phi_eta_alt,tm_quark_info &le,int twall)
{insert_photon_on_the_source(prop,phi_eta_alt,all_dirs,le,twall);}

//insert the conserved current on the source
void insert_conserved_current_on_the_source(spinspin *prop,coords dirs,tm_quark_info &le)
{
  GET_THREAD_ID();
  
  //phases
  complex phases[NDIM];
  for(int mu=0;mu<NDIM;mu++)
    {
      phases[mu][0]=cos(le.bc[mu]*M_PI);
      phases[mu][1]=sin(le.bc[mu]*M_PI);
    }
  
  //copy on the temporary and communicate borders
  vector_copy(temp_lep,prop);
  communicate_lx_spinspin_borders(temp_lep);
  vector_reset(prop);
  
  dirac_matr GAMMA;
  if(pure_wilson) dirac_prod_double(&GAMMA,base_gamma+0,1);
  else dirac_prod_idouble(&GAMMA,base_gamma+5,-tau3[le.r]);
  
  //prepare each propagator for a single lepton
  //by computing (phi(x+mu)(-i t3 g5+gmu)/2-phi(x-mu)(-i t3 g5-gmu)/2)
  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
    for(int mu=0;mu<NDIM;mu++)
      {
	//find neighbors
	int ifw=loclx_neighup[ivol][mu];
	int ibw=loclx_neighdw[ivol][mu];
	
	//compute phase factor
	spinspin ph_bw,ph_fw;
	
	//transport down and up
	if(glb_coord_of_loclx[ivol][mu]==glb_size[mu]-1) unsafe_spinspin_prod_complex_conj2(ph_fw,temp_lep[ifw],phases[mu]);
	else spinspin_copy(ph_fw,temp_lep[ifw]);
	if(glb_coord_of_loclx[ivol][mu]==0) unsafe_spinspin_prod_complex(ph_bw,temp_lep[ibw],phases[mu]);
	else spinspin_copy(ph_bw,temp_lep[ibw]);
	
	//fix coefficients
	//also dir selection is made here
	spinspin_prodassign_double(ph_fw,+0.5*dirs[mu]);
	spinspin_prodassign_double(ph_bw,-0.5*dirs[mu]);
	
	//summ and subtract the two
	spinspin fw_M_bw,fw_P_bw;
	spinspin_subt(fw_M_bw,ph_fw,ph_bw);
	spinspin_summ(fw_P_bw,ph_fw,ph_bw);
	
	//put -i t3 g5 on the summ
	spinspin temp_P;
	unsafe_spinspin_prod_dirac(temp_P,fw_P_bw,&GAMMA);
	spinspin_summassign(prop[ivol],temp_P);
	
	//put gmu on the fw-bw diff
	spinspin temp_M;
	unsafe_spinspin_prod_dirac(temp_M,fw_M_bw,base_gamma+map_mu[mu]);
	spinspin_summassign(prop[ivol],temp_M);
      }
  set_borders_invalid(prop);
}

//insert the conserved current on the sink
void insert_conserved_current_on_the_sink(spinspin *prop,coords dirs,tm_quark_info &le)
{
  GET_THREAD_ID();
  
  //phases
  complex phases[NDIM];
  for(int mu=0;mu<NDIM;mu++)
    {
      phases[mu][0]=cos(-le.bc[mu]*M_PI);
      phases[mu][1]=sin(-le.bc[mu]*M_PI);
    }
  
  //copy on the temporary and communicate borders
  vector_copy(temp_lep,prop);
  communicate_lx_spinspin_borders(temp_lep);
  vector_reset(prop);
  
  dirac_matr GAMMA;
  if(pure_wilson) dirac_prod_double(&GAMMA,base_gamma+0,1);
  else dirac_prod_idouble(&GAMMA,base_gamma+5,-tau3[le.r]);
  
  //prepare each propagator for a single lepton
  //by computing (phi(x-mu)(-i t3 g5+gmu)/2-phi(x+mu)(-i t3 g5-gmu)/2)
  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
    for(int mu=0;mu<NDIM;mu++)
      {
	//find neighbors
	int ifw=loclx_neighup[ivol][mu];
	int ibw=loclx_neighdw[ivol][mu];
	
	//compute phase factor
	spinspin ph_bw,ph_fw;
	
	//transport down and up
	if(glb_coord_of_loclx[ivol][mu]==glb_size[mu]-1) unsafe_spinspin_prod_complex_conj2(ph_fw,temp_lep[ifw],phases[mu]);
	else spinspin_copy(ph_fw,temp_lep[ifw]);
	if(glb_coord_of_loclx[ivol][mu]==0) unsafe_spinspin_prod_complex(ph_bw,temp_lep[ibw],phases[mu]);
	else spinspin_copy(ph_bw,temp_lep[ibw]);
	
	//fix coefficients
	//also dir selection is made here
	spinspin_prodassign_double(ph_fw,-0.5*dirs[mu]);
	spinspin_prodassign_double(ph_bw,+0.5*dirs[mu]);
	
	//summ and subtract the two
	spinspin fw_M_bw,fw_P_bw;
	spinspin_subt(fw_M_bw,ph_fw,ph_bw);
	spinspin_summ(fw_P_bw,ph_fw,ph_bw);
	
	//put -i t3 g5 on the summ
	spinspin temp_P;
	unsafe_spinspin_prod_dirac(temp_P,fw_P_bw,&GAMMA);
	spinspin_summassign(prop[ivol],temp_P);
	
	//put -gmu on the fw-bw diff
	spinspin temp_M;
	unsafe_spinspin_prod_dirac(temp_M,fw_M_bw,base_gamma+map_mu[mu]);
	spinspin_subtassign(prop[ivol],temp_M);
      }
  set_borders_invalid(prop);
}

//generate all the lepton propagators, pointing outward
//the computations is done by:
// 1)putting the correct phase in x space, given by exp(E_mu*t-i*vec(p)*vec(x))
// 2)multiplying it by the conserved current inserting eta or phi
// 3)going to momentum space
// 4)multiplying by the lepton propagator in momentum space
// 5)coming back to x space
THREADABLE_FUNCTION_1ARG(generate_lepton_propagators, int,t2)
{
  GET_THREAD_ID();
  
  if(IS_MASTER_THREAD) lepton_prop_time-=take_time();
  master_printf("Generating lepton propagators for time %d\n",t2); //added for chris
  
  for(int ilepton=0;ilepton<nleptons;ilepton++)
    for(int ori=0;ori<norie;ori++)
      for(int phi_eta_alt=0;phi_eta_alt<nphi_eta_alt;phi_eta_alt++)
	for(int r=0;r<nr;r++)
	  {
	    //set the properties of the meson
	    //time boundaries are anti-periodic, space are as for external line
	    tm_quark_info le=get_lepton_info(ilepton,ori,r);
	    
	    //select the propagator
	    int iprop=ilprop(ilepton,ori,phi_eta_alt,r,t2); //added for Chris
	    spinspin *prop=L[iprop];
	    
	    //put it to a phase
	    set_to_lepton_sink_phase_factor(prop,ilepton,le);
	    
	    //if we are doing Nazario's way (with the external line) add it
	    if(follow_chris_or_nazario==follow_nazario)
	      {
		//select only the wall
		int tmiddle=glb_size[0]/2;
		select_propagator_timeslice(prop,prop,tmiddle);
		multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
	      }
	    
	    //select only the wall
	    int twall;
	    if(t2<0||t2>=glb_size[0]) twall=-1;
	    else twall=t2;
	    insert_photon_on_the_source(prop,phi_eta_alt,le,twall);
	    multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
	    
	    double lep_norm;
	    double_vector_glb_scalar_prod(&lep_norm,(double*)prop,(double*)prop,sizeof(spinspin)/sizeof(double)*loc_vol);
	    master_printf("lep prop_norm2: %lg\n",lep_norm);
	  }
  
  if(IS_MASTER_THREAD) lepton_prop_time+=take_time();
}
THREADABLE_FUNCTION_END

//same without hadron and photon
THREADABLE_FUNCTION_0ARG(compute_lepton_free_loop)
{
  GET_THREAD_ID();
  
  FILE *fout=open_file(combine("%s/corr_l_free",outfolder).c_str(),"w");
  
  if(IS_MASTER_THREAD) lepton_prop_time-=take_time();
  master_printf("Generating free loop\n");
  spinspin *prop=nissa_malloc("prop",loc_vol+bord_vol,spinspin);
  complex *corr=nissa_malloc("corr",glb_size[0],complex);
  
  {
    master_printf("Projection of the lepton propagator (from right), should get c[1]=-0.00236099873290829\n");
    
    //fix lepton
    tm_quark_info le=get_lepton_info(0,0,0);
    
    //compute prop
    vector_reset(prop);
    spinspin_put_to_diag(prop[0],1);
    multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
    
    //projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    //correlator
    vector_reset(corr);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	//compute the phase
	complex ph={cos(arg),sin(arg)};
	safe_complex_prod(c,c,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d %16.16lg %16.16lg\n",t,corr[t][RE],corr[t][IM]);
    
    master_printf("------------------\n");
  }
  
  {
    master_printf("Projection of the lepton propagator (computed from the left) c[1]=1.075643001750072\n");
    
    //compute the propagator by multiplying from the left
    tm_quark_info le=get_lepton_info(0,0,0);
    vector_reset(prop);
    spinspin_put_to_diag(prop[0],1);
    multiply_from_left_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
    
    //projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    vector_reset(corr);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=-le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	//compute the phase
	complex ph={cos(arg),sin(arg)};
	safe_complex_prod(c,c,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d %16.16lg %16.16lg\n",t,corr[t][RE],corr[t][IM]);
    master_printf("------------------\n");
  }
  
  {
    master_printf("CHECK 18, Projection of the lepton propagator at rest \n[plain, time current, time conserved current, spat conserved current], we should get c[1]=[0.686606436935,0.683198939788,0.696862918583,0]\n");
    
    //fix lepton bc
    tm_quark_info le=get_lepton_info(0,0,0);
    for(int mu=0;mu<NDIM;mu++) le.bc[mu]=0;
    master_printf("Note that the ratio between not conserved and original should amount to m/sinh(E)=%12.12lg\n",le.mass/sinh(tm_quark_energy(le,0)));
    compute_x_space_twisted_propagator_by_fft(prop,le,base);
    
    //allocate prop*CV and prop*V
    spinspin *propCV=nissa_malloc("propCV",loc_vol+bord_vol,spinspin);
    spinspin *propCVi=nissa_malloc("propCVi",loc_vol+bord_vol,spinspin);
    spinspin *propV=nissa_malloc("propV",loc_vol+bord_vol,spinspin);
    vector_copy(propCV,prop);
    vector_copy(propCVi,prop);
    
    //insert time current
    coords time_dir={1,0,0,0};
    coords spat_dir={0,1,0,0};
    insert_conserved_current_on_the_sink(propCV,time_dir,le);
    insert_conserved_current_on_the_sink(propCVi,spat_dir,le);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol) safe_dirac_prod_spinspin(propV[ivol],base_gamma+4,prop[ivol]);
    THREAD_BARRIER();
    
    //take projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    //erase corr
    vector_reset(corr);
    complex *corrCV=nissa_malloc("corrCV",glb_size[0],complex);
    complex *corrCVi=nissa_malloc("corrCVi",glb_size[0],complex);
    complex *corrV=nissa_malloc("corrV",glb_size[0],complex);
    vector_reset(corrCV);
    vector_reset(corrCVi);
    vector_reset(corrV);
    
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c,cV,cCV,cCVi;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	trace_spinspin_prod_spinspin(cV,propV[ivol],pro);
	trace_spinspin_prod_spinspin(cCV,propCV[ivol],pro);
	trace_spinspin_prod_spinspin(cCVi,propCVi[ivol],pro);
	
	//phases
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=-le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	complex ph={cos(arg),sin(arg)};
	
	//fix phases and collapse
	safe_complex_prod(c,c,ph);
	safe_complex_prod(cV,cV,ph);
	safe_complex_prod(cCV,cCV,ph);
	safe_complex_prod(cCVi,cCVi,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
	complex_summassign(corrV[glb_coord_of_loclx[ivol][0]],cV);
	complex_summassign(corrCV[glb_coord_of_loclx[ivol][0]],cCV);
	complex_summassign(corrCVi[glb_coord_of_loclx[ivol][0]],cCVi);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d   %10.10lg %10.10lg  %10.10lg %10.10lg  %10.10lg   %10.10lg %10.10lg  %10.10lg   %10.10lg %10.10lg\n",
						 t,
						 corr[t][RE],corr[t][IM],
						 corrV[t][RE],corrV[t][IM],corrV[t][RE]/corr[t][RE],
						 corrCV[t][RE],corrCV[t][IM],corrCV[t][RE]/corr[t][RE],
						 corrCVi[t][RE],corrCVi[t][IM]);
    
    nissa_free(propCV);
    nissa_free(propCVi);
    nissa_free(propV);

    nissa_free(corrV);
    nissa_free(corrCV);
    nissa_free(corrCVi);
    
    master_printf("------------------\n");
  }
  
  {
    master_printf("CHECK 18b, Projection of the lepton propagator \n[plain, all conserved current]]\n");
    
    //fix lepton bc
    tm_quark_info le=get_lepton_info(0,0,0);
    le.bc[0]=1;
    le.bc[1]=0.2;
    le.bc[2]=0.4;
    le.bc[3]=0.6;
    master_printf("Note that the ratio between not conserved and original should amount to 0.5990580951\n");
    compute_x_space_twisted_propagator_by_fft(prop,le,base);
    
    //allocate prop*CV and prop*V
    spinspin *propCV=nissa_malloc("propCV",loc_vol+bord_vol,spinspin);
    vector_copy(propCV,prop);
    
    insert_conserved_current_on_the_sink(propCV,all_dirs,le);
    
    //take projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    //erase corr
    vector_reset(corr);
    complex *corrCV=nissa_malloc("corrCV",glb_size[0],complex);
    vector_reset(corrCV);
    
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c,cCV;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	trace_spinspin_prod_spinspin(cCV,propCV[ivol],pro);
	
	//phases
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=-le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	complex ph={cos(arg),sin(arg)};
	
	//fix phases and collapse
	safe_complex_prod(c,c,ph);
	safe_complex_prod(cCV,cCV,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
	complex_summassign(corrCV[glb_coord_of_loclx[ivol][0]],cCV);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d   %10.10lg %10.10lg  %10.10lg %10.10lg  %10.10lg\n",
						 t,
						 corr[t][RE],corr[t][IM],
						 corrCV[t][RE],corrCV[t][IM],corrCV[t][RE]/corr[t][RE]);
    nissa_free(propCV);
    nissa_free(corrCV);
    
    master_printf("------------------\n");
  }

  {
    master_printf("Projection of the lepton propagator AT REST multiplied by conserved current on the sink, should get c[1]=0.6968629185832987\n");
    
    //fix lepton to rest
    tm_quark_info le=get_lepton_info(0,0,0);
    for(int mu=0;mu<NDIM;mu++) le.bc[mu]=0;
    
    //compute the propagator
    compute_x_space_twisted_propagator_by_fft(prop,le,base);
    
    //insert time conserved current
    coords time_dir={1,0,0,0};
    insert_conserved_current_on_the_sink(prop,time_dir,le);
    
    //projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    //compute the correlator
    vector_reset(corr);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=-le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	//compute the phase
	complex ph={cos(arg),sin(arg)};
	safe_complex_prod(c,c,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d %16.16lg %16.16lg\n",t,corr[t][RE],corr[t][IM]);
    
    master_printf("------------------\n");
  }
  
  for(int r=0;r<nr;r++)
  {
    master_printf("Projection of the lepton propagator (r=%d) multiplied by time conserved current on the source, should get c[1]=-0.001147877620189829\n",r);
    
    //fix lepton
    tm_quark_info le=get_lepton_info(0,0,r);
    
    //compute prop
    vector_reset(prop);
    spinspin_put_to_diag(prop[0],1);
    multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
    
    //insert current on the source
    coords time_dir={1,0,0,0};
    insert_conserved_current_on_the_source(prop,time_dir,le);
    
    //projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    //correlator
    vector_reset(corr);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	//compute the phase
	complex ph={cos(arg),sin(arg)};
	safe_complex_prod(c,c,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d %16.16lg %16.16lg\n",t,corr[t][RE],corr[t][IM]);
    
    master_printf("------------------\n");
  }

  for(int r=0;r<nr;r++)
  {
    master_printf("Projection of the lepton propagator (r=%d) multiplied by full conserved current on the source, should get c[1]=-0.001147877620189519\n",r);
    
    //fix lepton
    tm_quark_info le=get_lepton_info(0,0,r);
    
    //compute prop
    vector_reset(prop);
    spinspin_put_to_diag(prop[0],1);
    multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
    
    //insert current on the source
    insert_conserved_current_on_the_source(prop,all_dirs,le);
    
    //projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    //correlator
    vector_reset(corr);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	//compute the phase
	complex ph={cos(arg),sin(arg)};
	safe_complex_prod(c,c,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d %16.16lg %16.16lg\n",t,corr[t][RE],corr[t][IM]);
    
    master_printf("------------------\n");
  }

  for(int r=0;r<nr;r++)
  {
    master_printf("Projection of the lepton propagator (r=%d) multiplied by full photon \"i\" field on the source, should get c[1]=-0.001147877620189519 as before\n",r);
    
    //fix lepton
    tm_quark_info le=get_lepton_info(0,0,r);
    
    //compute prop
    vector_reset(prop);
    spinspin_put_to_diag(prop[0],1);
    multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
    
    //insert current on the source
    spin1field *i_vect=nissa_malloc("i_vect",loc_vol,spin1field);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol) for(int i=0;i<4;i++) complex_put_to_imag(i_vect[ivol][i],1);
    insert_photon_on_the_source(prop,i_vect,all_dirs,le,-1);
    nissa_free(i_vect);
    
    //projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    //correlator
    vector_reset(corr);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	//compute the phase
	complex ph={cos(arg),sin(arg)};
	safe_complex_prod(c,c,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d %16.16lg %16.16lg\n",t,corr[t][RE],corr[t][IM]);
    
    master_printf("------------------\n");
  }

  {
    master_printf("Projection of the lepton propagator (computed from the left) multiplied by conserved current on the sink c[1]=0.5229594204404844\n");
    
    //compute the propagator by multiplying from the left
    tm_quark_info le=get_lepton_info(0,0,0);
    vector_reset(prop);
    spinspin_put_to_diag(prop[0],1);
    multiply_from_left_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
    
    //insert time conserved current
    coords time_dir={1,0,0,0};
    insert_conserved_current_on_the_sink(prop, time_dir, le);
    
    //projector
    spinspin pro;
    twisted_on_shell_operator_of_imom(pro,le,0,false,-1,base);
    
    vector_reset(corr);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	complex c;
	trace_spinspin_prod_spinspin(c,prop[ivol],pro);
	double arg=0;
	for(int mu=1;mu<NDIM;mu++)
	  arg+=-le.bc[mu]*M_PI/glb_size[mu]*glb_coord_of_loclx[ivol][mu];
	//compute the phase
	complex ph={cos(arg),sin(arg)};
	safe_complex_prod(c,c,ph);
	complex_summassign(corr[glb_coord_of_loclx[ivol][0]],c);
      }
    THREAD_BARRIER();
    for(int t=0;t<glb_size[0];t++) master_printf("%d %16.16lg %16.16lg\n",t,corr[t][RE],corr[t][IM]);
    master_printf("------------------\n");
  }
  
  {
    master_printf("comparing colorless and colorful application of conserved current on a random vector\n");
    
    //fix lepton bc to zero
    tm_quark_info le=get_lepton_info(0,0,0);
    coords time_dir={1,0,0,0};
    for(int mu=0;mu<NDIM;mu++) le.bc[mu]=0;
    
    //create propagator and promote it to su3
    vector_reset(prop);
    su3spinspin *spro=nissa_malloc("spro",loc_vol,su3spinspin);
    vector_reset(spro);
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
      {
	double p=rnd_get_gauss_double(loc_rnd_gen+ivol);
	//p=1;
	spinspin_put_to_diag(prop[ivol],p);
	for(int ic=0;ic<NCOL;ic++) spinspin_put_to_diag(spro[ivol][ic][ic],p);
      }
    set_borders_invalid(prop);
    set_borders_invalid(spro);
    
    //generate the unitary conf
    quad_su3 *cc=nissa_malloc("uconf",loc_vol,quad_su3);
    generate_cold_lx_conf(cc);
    
    //apply
    su3spinspin *sproc=nissa_malloc("sproc",loc_vol,su3spinspin);
    if(base==MAX_TWIST_BASE) insert_tm_conserved_current(sproc,cc,spro,le.r,time_dir,-1);
    else insert_wilson_conserved_current(sproc,cc,spro,time_dir,-1);
    insert_conserved_current_on_the_sink(prop,time_dir,le);
    
    //for(int ivol=0;ivol<loc_vol;ivol++)
    int ivol=1;
    {
      master_printf("ivol: %d\n",ivol);
      for(int id=0;id<4;id++)
	for(int jd=0;jd<4;jd++)
	  for(int ri=0;ri<2;ri++)
	    master_printf("%d %d %d %12.12lg\t%12.12lg\n",id,jd,ri,prop[ivol][id][jd][ri],sproc[ivol][0][0][id][jd][ri]);
      master_printf("---\n");
    }
    
    nissa_free(cc);
    nissa_free(spro);
    nissa_free(sproc);
  }
  
    //crash("ciao");
      
  for(int ilepton=0;ilepton<nleptons;ilepton++)
    for(int orie=0;orie<norie;orie++)
      for(int rl=0;rl<nr;rl++)
	{
	  //set the properties of the meson
	  //time boundaries are anti-periodic, space are as for external line
	  tm_quark_info le=get_lepton_info(ilepton,orie,rl);
	  master_printf("le.bc[1]: %lg\n",le.bc[1]);
	  //put it to a phase
	  set_to_lepton_sink_phase_factor(prop,ilepton,le);
	  int twall=glb_size[0]/2;
	  select_propagator_timeslice(prop,prop,twall);
	  
	  //multiply with the lepton prop
	  multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
	  insert_conserved_current_on_the_source(prop,all_dirs,le);
	  //insert_conserved_current_on_the_source(prop,ilepton,time_dir,le);
	  multiply_from_right_by_x_space_twisted_propagator_by_fft(prop,prop,le,base);
	  
	  //get the projectors
	  spinspin promu[2],pronu[2];
	  twisted_on_shell_operator_of_imom(promu[0],le,0,false,-1,base);
	  twisted_on_shell_operator_of_imom(promu[1],le,0,false,+1,base);
	  naive_massless_on_shell_operator_of_imom(pronu[0],le.bc,0,-1);
	  naive_massless_on_shell_operator_of_imom(pronu[1],le.bc,0,+1);
	  
	  //compute the right part of the leptonic loop: G0 G^dag
	  dirac_matr hadrolept_proj_gamma[nhadrolept_proj];
	  for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
	    {
	      int ig=hadrolept_projs[ig_proj];
	      dirac_matr temp_gamma;
	      dirac_herm(&temp_gamma,base_gamma+ig);
	      dirac_prod(hadrolept_proj_gamma+ig_proj,base_gamma+map_mu[0],&temp_gamma);
	    }
	  
	  for(int ins=0;ins<nweak_ins;ins++)
	    {
	      //define a local storage
	      spinspin l_loc_corr[loc_size[0]];
	      for(int i=0;i<loc_size[0];i++) spinspin_put_to_zero(l_loc_corr[i]);
	      
	      NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
		{
		  int t=loc_coord_of_loclx[ivol][0];
		  
		  //multiply lepton side on the right (source) side
		  spinspin l;
		  unsafe_spinspin_prod_dirac(l,prop[ivol],base_gamma+list_weak_insl[ins]);
		  
		  //add the neutrino phase
		  complex ph;
		  get_antineutrino_source_phase_factor(ph,ivol,ilepton,le.bc);
		  spinspin_summ_the_complex_prod(l_loc_corr[t],l,ph);
		}
	      glb_threads_reduce_double_vect((double*)l_loc_corr,loc_size[0]*sizeof(spinspin)/sizeof(double));
	      
	      //save projection on LO
	      for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
		{
		  vector_reset(corr);
		  NISSA_PARALLEL_LOOP(loc_t,0,loc_size[0])
		    {
		      int glb_t=loc_t+rank_coord[0]*loc_size[0];
		      int ilnp=(glb_t>=glb_size[0]/2); //select the lepton/neutrino projector
		      
		      spinspin td;
		      unsafe_spinspin_prod_spinspin(td,l_loc_corr[loc_t],pronu[ilnp]);
		      spinspin dtd;
		      unsafe_spinspin_prod_spinspin(dtd,promu[ilnp],td);
		      trace_spinspin_with_dirac(corr[glb_t],dtd,hadrolept_proj_gamma+ig_proj);
		      complex_prodassign_double(corr[glb_t],1.0/glb_spat_vol);
		    }
		  THREAD_BARRIER();
		  
		  if(IS_MASTER_THREAD)
		    {
		      glb_nodes_reduce_complex_vect(corr,glb_size[0]);
		      master_fprintf(fout," # ilepton=%d, orie=%d, rl=%d, ins=%s, ig_proj=%s\n\n",ilepton,orie,rl,gtag[list_weak_insl[ins]],gtag[hadrolept_projs[ig_proj]]);
		      for(int t=0;t<glb_size[0];t++)
			master_fprintf(fout,"%+016.016lg %+016.016lg\n",corr[t][0],corr[t][1]);
		      master_fprintf(fout,"\n");
		    }
		}
	      if(IS_MASTER_THREAD) nlept_contr_tot+=nhadrolept_proj;
	      THREAD_BARRIER();
	    }
	}
  
  nissa_free(prop);
  nissa_free(corr);
  close_file(fout);
  
  if(IS_MASTER_THREAD) lepton_prop_time+=take_time();
}
THREADABLE_FUNCTION_END

////////////////////////////////////////// purely hadronic correlators ///////////////////////////////////////////

//compute all the hadronic correlations
void compute_hadronic_correlations()
{
  master_printf("Computing hadronic correlation functions\n");
  
  hadr_contr_time-=take_time();
  for(int icombo=0;icombo<ncombo_hadr_corr;icombo++)
    for(int imass=0;imass<nqmass;imass++)
      for(int jmass=0;jmass<nqmass;jmass++)
	for(int r=0;r<nr;r++)
	  {
	    //compute the correlation function
	    meson_two_points_Wilson_prop(glb_corr,loc_corr,ig_hadr_so,Q[iqprop(imass,prop1_hadr_map[icombo],r)],ig_hadr_si,Q[iqprop(jmass,prop2_hadr_map[icombo],r)],nhadr_contr);
	    nhadr_contr_tot+=nhadr_contr;
	    
	    //save to the total stack
	    for(int ihadr_contr=0;ihadr_contr<nhadr_contr;ihadr_contr++)
	      for(int t=0;t<glb_size[0];t++)
		{
		  int i=t+glb_size[0]*(ihadr_contr+nhadr_contr*(r+nr*(jmass+nqmass*(imass+nqmass*icombo))));
		  complex_summassign(hadr_corr[i],glb_corr[t+glb_size[0]*ihadr_contr]);
		  //master_printf("%d %d %lg %lg\n",t,i,hadr_corr[i][RE],glb_corr[t+glb_size[0]*ihadr_contr][RE]);
		}
	  }
  hadr_contr_time+=take_time();
}

/////////////////////////////////////////////// hadroleptonic correlators //////////////////////////////////////////

//compute the hadronic part of the lepton correlation function
//as usual, FIRST propagator is reverted
THREADABLE_FUNCTION_3ARG(hadronic_part_leptonic_correlation, spinspin*,hadr, PROP_TYPE*,S1, PROP_TYPE*,S2)
{
  GET_THREAD_ID();
  
  vector_reset(hadr);
  
  //it's just the matter of inserting gamma5*gamma5=identity between S1^dag and S2
  //on the sink gamma5 must still be inserted!
  NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
    for(int ic_si=0;ic_si<NCOL;ic_si++)
#ifdef POINT_SOURCE_VERSION
      for(int ic_so=0;ic_so<NCOL;ic_so++)
#endif
	for(int id_si1=0;id_si1<4;id_si1++)
	  for(int id_si2=0;id_si2<4;id_si2++)
	    for(int id_so=0;id_so<4;id_so++)
	      complex_summ_the_conj1_prod
		(hadr[ivol][id_si2][id_si1], //this way when taking the trace with dirac matrix, that is acting on S2, as it should
#ifdef POINT_SOURCE_VERSION
		 S1[ivol][ic_si][ic_so][id_si1][id_so],S2[ivol][ic_si][ic_so][id_si2][id_so])
#else
                 S1[ivol][ic_si][id_si1][id_so],S2[ivol][ic_si][id_si2][id_so])
#endif
		 ;
  THREAD_BARRIER();
}
THREADABLE_FUNCTION_END

//compute the leptonic part of the correlation function
THREADABLE_FUNCTION_6ARG(attach_leptonic_correlation, spinspin*,hadr, int,iprop, int,ilepton, int,orie, int,rl, int,ext_ind)
{
  GET_THREAD_ID();
  
  vector_reset(loc_corr);
  
  //get the lepton info and prop
  tm_quark_info le=get_lepton_info(ilepton,orie,rl);
  spinspin *lept=L[iprop];
  
  //get the projectors
  spinspin promu[2],pronu[2];
  twisted_on_shell_operator_of_imom(promu[0],le,0,false,-1,base);
  if(follow_chris_or_nazario==follow_nazario) twisted_on_shell_operator_of_imom(promu[1],le,0,false,+1,base);
  else twisted_on_shell_operator_of_imom(promu[1],le,0,false,-1,base);
  naive_massless_on_shell_operator_of_imom(pronu[0],le.bc,0,-1);
  if(follow_chris_or_nazario==follow_nazario) naive_massless_on_shell_operator_of_imom(pronu[1],le.bc,0,+1);
  else naive_massless_on_shell_operator_of_imom(pronu[1],le.bc,0,-1);
  if(follow_chris_or_nazario==follow_chris)
    for(int i=0;i<2;i++)
      safe_spinspin_prod_dirac(promu[i],promu[i],base_gamma+map_mu[0]);
  
  //compute the right part of the leptonic loop: G0 G^dag
  dirac_matr hadrolept_proj_gamma[nhadrolept_proj];
  for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
    {
      int ig=hadrolept_projs[ig_proj];
      dirac_matr temp_gamma;
      dirac_herm(&temp_gamma,base_gamma+ig);
      dirac_prod(hadrolept_proj_gamma+ig_proj,base_gamma+map_mu[0],&temp_gamma);
    }
  //insert gamma5 on the sink-hadron-gamma: S1^dag G5 GW S2 (G5 G5) - will dag and commutator with g0 come into?
  dirac_matr weak_ins_hadr_gamma[nweak_ins];
  for(int ins=0;ins<nweak_ins;ins++) dirac_prod(weak_ins_hadr_gamma+ins,base_gamma+5,base_gamma+list_weak_insq[ins]);
  
  for(int ins=0;ins<nweak_ins;ins++)
    {
      //define a local storage
      spinspin hl_loc_corr[loc_size[0]];
      for(int i=0;i<loc_size[0];i++) spinspin_put_to_zero(hl_loc_corr[i]);
      
      NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	{
	  int t=loc_coord_of_loclx[ivol][0];
	  
	  //multiply lepton side on the right (source) side
	  spinspin l;
	  unsafe_spinspin_prod_dirac(l,lept[ivol],base_gamma+list_weak_insl[ins]);
	  
	  //trace hadron side
	  complex h;
	  trace_spinspin_with_dirac(h,hadr[ivol],weak_ins_hadr_gamma+ins);
	  //get the neutrino phase (multiply hadron side) - notice that the sign of momentum is internally reversed
	  complex ph;
	  get_antineutrino_source_phase_factor(ph,ivol,ilepton,le.bc);
	  complex_prodassign(h,ph);
	  spinspin_summ_the_complex_prod(hl_loc_corr[t],l,h);
	}
      glb_threads_reduce_double_vect((double*)hl_loc_corr,loc_size[0]*sizeof(spinspin)/sizeof(double));
      
      //save projection on LO
      for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
	NISSA_PARALLEL_LOOP(loc_t,0,loc_size[0])
	  {
	    int glb_t=loc_t+rank_coord[0]*loc_size[0];
	    int ilnp=(glb_t>=glb_size[0]/2); //select the lepton/neutrino projector
	    
	    spinspin td;
	    unsafe_spinspin_prod_spinspin(td,hl_loc_corr[loc_t],pronu[ilnp]);
	    spinspin dtd;
	    unsafe_spinspin_prod_spinspin(dtd,promu[ilnp],td);
	    complex hl;
	    trace_spinspin_with_dirac(hl,dtd,hadrolept_proj_gamma+ig_proj);
	    
	    //summ the average
	    int i=glb_t+glb_size[0]*(ig_proj+nhadrolept_proj*(list_weak_ind_contr[ins]+nweak_ind*ext_ind));
	    complex_summ_the_prod_double(hadrolept_corr[i],hl,1.0/glb_spat_vol); //here to remove the statistical average on xw
	  }
      if(IS_MASTER_THREAD) nlept_contr_tot+=nhadrolept_proj;
      THREAD_BARRIER();
    }
}
THREADABLE_FUNCTION_END

//compute the leptonic part of the correlation function
THREADABLE_FUNCTION_8ARG(attach_leptonic_correlation_chris, spinspin*,hadr, int,iprop, int,ilepton, int,orie, int,rl, int,ext_ind, int,t1, int,t2)
{
  GET_THREAD_ID();
  
  vector_reset(loc_corr);
  
  //get the lepton info and prop
  tm_quark_info le=get_lepton_info(ilepton,orie,rl);
  spinspin *lept=L[iprop];
  
  //get the projectors
  spinspin promu[2],pronu[2];
  twisted_on_shell_operator_of_imom(promu[0],le,0,false,-1,base);
  if(follow_chris_or_nazario==follow_nazario) twisted_on_shell_operator_of_imom(promu[1],le,0,false,+1,base);
  else twisted_on_shell_operator_of_imom(promu[1],le,0,false,-1,base);
  naive_massless_on_shell_operator_of_imom(pronu[0],le.bc,0,-1);
  if(follow_chris_or_nazario==follow_nazario) naive_massless_on_shell_operator_of_imom(pronu[1],le.bc,0,+1);
  else naive_massless_on_shell_operator_of_imom(pronu[1],le.bc,0,-1);
  if(follow_chris_or_nazario==follow_chris)
    for(int i=0;i<2;i++)
      safe_spinspin_prod_dirac(promu[i],promu[i],base_gamma+map_mu[0]);
  
  //compute the right part of the leptonic loop: G0 G^dag
  dirac_matr hadrolept_proj_gamma[nhadrolept_proj];
  for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
    {
      int ig=hadrolept_projs[ig_proj];
      dirac_matr temp_gamma;
      dirac_herm(&temp_gamma,base_gamma+ig);
      dirac_prod(hadrolept_proj_gamma+ig_proj,base_gamma+map_mu[0],&temp_gamma);
    }
  //insert gamma5 on the sink-hadron-gamma: S1^dag G5 GW S2 (G5 G5) - will dag and commutator with g0 come into?
  dirac_matr weak_ins_hadr_gamma[nweak_ins];
  for(int ins=0;ins<nweak_ins;ins++) dirac_prod(weak_ins_hadr_gamma+ins,base_gamma+5,base_gamma+list_weak_insq[ins]);
  
  for(int ins=0;ins<nweak_ins;ins++)
    {
      //define a local storage
      spinspin hl_loc_corr[loc_size[0]];
      for(int i=0;i<loc_size[0];i++) spinspin_put_to_zero(hl_loc_corr[i]);
      
      NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	{
	  int t=loc_coord_of_loclx[ivol][0];
	  
	  //multiply lepton side on the right (source) side
	  spinspin l;
	  unsafe_spinspin_prod_dirac(l,lept[ivol],base_gamma+list_weak_insl[ins]);
	  
	  //trace hadron side
	  complex h;
	  trace_spinspin_with_dirac(h,hadr[ivol],weak_ins_hadr_gamma+ins);
	  //get the neutrino phase (multiply hadron side) - notice that the sign of momentum is internally reversed
	  complex ph;
	  get_antineutrino_source_phase_factor(ph,ivol,ilepton,le.bc);
	  complex_prodassign(h,ph);
	  spinspin_summ_the_complex_prod(hl_loc_corr[t],l,h);
	}
      glb_threads_reduce_double_vect((double*)hl_loc_corr,loc_size[0]*sizeof(spinspin)/sizeof(double));
      
      //save projection on LO
      for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
	NISSA_PARALLEL_LOOP(loc_t,0,loc_size[0])
	  {
	    int glb_t=loc_t+rank_coord[0]*loc_size[0];
	    int ilnp=(glb_t>=glb_size[0]/2); //select the lepton/neutrino projector
	    
	    spinspin td;
	    unsafe_spinspin_prod_spinspin(td,hl_loc_corr[loc_t],pronu[ilnp]);
	    spinspin dtd;
	    unsafe_spinspin_prod_spinspin(dtd,promu[ilnp],td);
	    complex hl;
	    trace_spinspin_with_dirac(hl,dtd,hadrolept_proj_gamma+ig_proj);
	    
	    //summ the average
	    if(glb_t==chris_t)
	      {
		int i=t1+glb_size[0]*(t2+glb_size[0]*(ig_proj+nhadrolept_proj*(list_weak_ind_contr[ins]+nweak_ind*ext_ind)));
		complex_summ_the_prod_double(hadrolept_corr_chris[i],hl,1.0/glb_spat_vol);
	      }
	  }
      if(IS_MASTER_THREAD) nlept_contr_tot+=nhadrolept_proj;
      THREAD_BARRIER();
    }
}
THREADABLE_FUNCTION_END

//do not attach the leptonic part
THREADABLE_FUNCTION_2ARG(do_not_attach_leptonic_correlation, spinspin*,hadr, int,ext_ind)
{
  GET_THREAD_ID();
  
  vector_reset(loc_corr);
  
  //insert gamma5 on the sink-hadron-gamma: S1^dag G5 GW S2 (G5 G5) - will dag and commutator with g0 come into?
  dirac_matr weak_ins_hadr_gamma[nweak_ins];
  for(int ins=0;ins<nweak_ins;ins++) dirac_prod(weak_ins_hadr_gamma+ins,base_gamma+5,base_gamma+list_weak_insq[ins]);
  
  for(int ins=0;ins<nweak_ins;ins++)
    {
      //define a local storage
      complex h_loc_corr[loc_size[0]];
      for(int i=0;i<loc_size[0];i++) complex_put_to_zero(h_loc_corr[i]);
      
      NISSA_PARALLEL_LOOP(ivol,0,loc_vol)
	{
	  int t=loc_coord_of_loclx[ivol][0];
	  
	  //trace hadron side
	  complex h;
	  trace_spinspin_with_dirac(h,hadr[ivol],weak_ins_hadr_gamma+ins);
	  complex_summassign(h_loc_corr[t],h);
	}
      glb_threads_reduce_double_vect((double*)h_loc_corr,loc_size[0]*sizeof(complex)/sizeof(double));
      
      //save projection on LO
      NISSA_PARALLEL_LOOP(loc_t,0,loc_size[0])
	{
	  int glb_t=loc_t+rank_coord[0]*loc_size[0];
	  
	  //summ the average
	  int i=glb_t+glb_size[0]*(0+nhadrolept_proj*(list_weak_ind_contr[ins]+nweak_ind*ext_ind));
	  complex_summassign(hadrolept_corr[i],h_loc_corr[loc_t]);
	}
      if(IS_MASTER_THREAD) nlept_contr_tot+=nhadrolept_proj;
      THREAD_BARRIER();
    }
}
THREADABLE_FUNCTION_END

//compute the total hadroleptonic correlation functions
void compute_hadroleptonic_correlations()
{
  master_printf("Computing leptonic correlation functions\n");
  lept_contr_time-=take_time();
  
  int ind=0;
  for(int ilepton=0;ilepton<nleptons;ilepton++)
    for(int qins=0;qins<2;qins++)
      for(int irev=0;irev<2;irev++)
	for(int iphi_eta_alt=0;iphi_eta_alt<nphi_eta_alt;iphi_eta_alt++)
	  for(int r2=0;r2<nr;r2++)
	    {
	      //takes the index of the quarks
	      int iq1=lep_corr_iq1[ilepton];
	      int iq2=lep_corr_iq2[ilepton];
	      
	      //takes the propagators
	      qprop_t PROP1_TYPE,PROP2_TYPE;
	      //ANNA2
	      if(qins==0)
	      {
		PROP1_TYPE=PROP_PHI_ETA_ALT[iphi_eta_alt];
		PROP2_TYPE=PROP_0;
	      }
	      else
	      {
		PROP1_TYPE=PROP_0;
		PROP2_TYPE=PROP_PHI_ETA_ALT[iphi_eta_alt];
	      }
	      int ip1=iqprop(iq1,PROP1_TYPE,r2);
	      int ip2=iqprop(iq2,PROP2_TYPE,r2);
	      
	      if(irev==1) std::swap(ip1,ip2); //select the propagator to revert
	      
	      //compute the hadronic part
	      hadronic_part_leptonic_correlation(hadr,Q[ip1],Q[ip2]);
	      
	      for(int orie=0;orie<2;orie++)
		for(int rl=0;rl<nr;rl++)
		  {
		    //contract with lepton
		    //ANNA2 //added for chris
		    int PHI_ETA_ALT_bis[nphi_eta_alt]={ieta,iphi,ialt};
		    int iprop=ilprop(ilepton,orie,PHI_ETA_ALT_bis[iphi_eta_alt],rl,glb_size[0]); //notice inversion of phi/eta w.r.t hadron side
		    attach_leptonic_correlation(hadr,iprop,ilepton,orie,rl,ind);
		    //do_not_attach_leptonic_correlation(hadr,ind);
		    ind++;
		  }
	    }
  
  lept_contr_time+=take_time();
}

//compute the total hadroleptonic correlation functions
void compute_hadroleptonic_correlations_chris(int t1,int t2)
{
  master_printf("Computing leptonic correlation functions for Chris, t1=%d t2=%d\n",t1,t2);
  lept_contr_time-=take_time();
  
  int ind=0;
  for(int ilepton=0;ilepton<nleptons;ilepton++)
    for(int qins=0;qins<2;qins++)
      for(int irev=0;irev<2;irev++)
	for(int iphi_eta_alt=0;iphi_eta_alt<nphi_eta_alt;iphi_eta_alt++)
	  for(int r2=0;r2<nr;r2++)
	    {
	      //takes the index of the quarks
	      int iq1=lep_corr_iq1[ilepton];
	      int iq2=lep_corr_iq2[ilepton];
	      
	      //takes the propagators
	      qprop_t PROP1_TYPE,PROP2_TYPE;
	      //ANNA2
	      if(qins==0)
	      {
		PROP1_TYPE=PROP_PHI_ETA_ALT[iphi_eta_alt];
		PROP2_TYPE=PROP_0;
	      }
	      else
	      {
		PROP1_TYPE=PROP_0;
		PROP2_TYPE=PROP_PHI_ETA_ALT[iphi_eta_alt];
		}
	      int ip1=iqprop(iq1,PROP1_TYPE,r2);
	      int ip2=iqprop(iq2,PROP2_TYPE,r2);
	      
	      if(irev==1) std::swap(ip1,ip2); //select the propagator to revert
	      
	      //compute the hadronic part
	      hadronic_part_leptonic_correlation(hadr,Q[ip1],Q[ip2]);
	      
	      for(int orie=0;orie<2;orie++)
		for(int rl=0;rl<nr;rl++)
		  {
		    //contract with lepton
		    //ANNA2
		    qprop_t PHI_ETA_ALT_bis[nphi_eta_alt]={PROP_ETA,PROP_PHI,PROP_ALT};
		    int iprop=ilprop(ilepton,orie,PHI_ETA_ALT_bis[iphi_eta_alt],rl,t2); //notice inversion of phi/eta w.r.t hadron side
		    attach_leptonic_correlation_chris(hadr,iprop,ilepton,orie,rl,ind,t1,t2);
		    //do_not_attach_leptonic_correlation(hadr,ind);
		    ind++;
		  }
	    }
  
  lept_contr_time+=take_time();
}

//print out correlations
void print_correlations()
{
  print_time-=take_time();
  
  //open file and reduce
  FILE *fout=open_file(combine("%s/corr_hl",outfolder).c_str(),"w");
  glb_nodes_reduce_complex_vect(hadrolept_corr,glb_size[0]*nweak_ind*nhadrolept_proj*nind);
  
  //write down
  int ext_ind=0;
  for(int ilepton=0;ilepton<nleptons;ilepton++)
    for(int qins=0;qins<2;qins++)
      for(int irev=0;irev<2;irev++)
	for(int iphi_eta_alt=0;iphi_eta_alt<nphi_eta_alt;iphi_eta_alt++)
	  for(int r2=0;r2<nr;r2++)
	    {
	      //takes the index of the quarks
	      int iq1=lep_corr_iq1[ilepton];
	      int iq2=lep_corr_iq2[ilepton];
	      if(irev==1) std::swap(iq1,iq2);
	      for(int orie=0;orie<2;orie++)
		for(int rl=0;rl<nr;rl++)
		  {
		    if(!pure_wilson) master_fprintf(fout," # mq1=%lg mq2=%lg qins=%d qrev=%d ins=%s rq1=%d rq2=%d lep_orie=%+d rl=%d\n\n",
						    qmass[iq1],qmass[iq2],qins+1,irev+1,photon_field_name[iphi_eta_alt],!r2,r2,sign_orie[orie],rl);
		    else             master_fprintf(fout," # kappaq1=%lg kappaq2=%lg qins=%d qrev=%d ins=%s lep_orie=%+d\n\n",
						    qkappa[iq1],qkappa[iq2],qins+1,irev+1,photon_field_name[iphi_eta_alt],sign_orie[orie]);
		    for(int ind=0;ind<nweak_ind;ind++)
		      for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
			{
			  master_fprintf(fout," # qins=%s lins=%s proj=%s\n\n",list_weak_ind_nameq[ind],list_weak_ind_namel[ind],gtag[hadrolept_projs[ig_proj]]);
			  for(int t=0;t<glb_size[0];t++)
			    {
			      int i=t+glb_size[0]*(ig_proj+nhadrolept_proj*(ind+nweak_ind*ext_ind));
			      master_fprintf(fout,"%+016.16lg %+016.16lg\n",hadrolept_corr[i][RE]/nsources,hadrolept_corr[i][IM]/nsources);
			    }
			  master_fprintf(fout,"\n");
			}
		    ext_ind++;
		  }
	    }
  close_file(fout);
  
  {
    //open file and reduce
    FILE *fout=open_file(combine("%s/corr_hl_chris",outfolder).c_str(),"w");
    glb_nodes_reduce_complex_vect(hadrolept_corr_chris,glb_size[0]*glb_size[0]*nweak_ind*nhadrolept_proj*nind);
    
    //write down
    int ext_ind=0;
    for(int ilepton=0;ilepton<nleptons;ilepton++)
      for(int qins=0;qins<2;qins++)
	for(int irev=0;irev<2;irev++)
	  for(int iphi_eta_alt=0;iphi_eta_alt<nphi_eta_alt;iphi_eta_alt++)
	    for(int r2=0;r2<nr;r2++)
	      {
		//takes the index of the quarks
		int iq1=lep_corr_iq1[ilepton];
		int iq2=lep_corr_iq2[ilepton];
		if(irev==1) std::swap(iq1,iq2);
		for(int orie=0;orie<2;orie++)
		  for(int rl=0;rl<nr;rl++)
		    {
		      if(!pure_wilson) master_fprintf(fout," # mq1=%lg mq2=%lg qins=%d qrev=%d ins=%s rq1=%d rq2=%d lep_orie=%+d rl=%d\n\n",
						      qmass[iq1],qmass[iq2],qins+1,irev+1,photon_field_name[iphi_eta_alt],!r2,r2,sign_orie[orie],rl);
		      else             master_fprintf(fout," # kappaq1=%lg kappaq2=%lg qins=%d qrev=%d ins=%s lep_orie=%+d\n\n",
						      qkappa[iq1],qkappa[iq2],qins+1,irev+1,photon_field_name[iphi_eta_alt],sign_orie[orie]);
		      for(int ind=0;ind<nweak_ind;ind++)
			for(int ig_proj=0;ig_proj<nhadrolept_proj;ig_proj++)
			  {
			    master_fprintf(fout," # qins=%s lins=%s proj=%s\n\n",list_weak_ind_nameq[ind],list_weak_ind_namel[ind],gtag[hadrolept_projs[ig_proj]]);
			    for(int t2=0;t2<glb_size[0];t2++)
			      {
				master_fprintf(fout," # t2=%d\n\n",t2);
				for(int t1=0;t1<glb_size[0];t1++)
				  {
				    int i=t1+glb_size[0]*(t2+glb_size[0]*(ig_proj+nhadrolept_proj*(ind+nweak_ind*ext_ind)));
				    master_fprintf(fout,"%+016.16lg %+016.16lg\n",hadrolept_corr_chris[i][RE]/nsources,hadrolept_corr_chris[i][IM]/nsources);
				  }
			      }
			    master_fprintf(fout,"\n");
			  }
		      ext_ind++;
		    }
	      }
    close_file(fout);
  }
  
  /////////////////////////////////// purely hadronic part ////////////////////////////////////////////
  
  //normalise
  double n=1.0/nsources;
  for(int i=0;i<hadr_corr_length;i++) complex_prodassign_double(hadr_corr[i],n);
  
  int ind=0;
  for(int icombo=0;icombo<ncombo_hadr_corr;icombo++)
    {
      fout=open_file(combine("%s/corr_%c%c",outfolder,prop_abbr[prop1_hadr_map[icombo]],prop_abbr[prop2_hadr_map[icombo]]).c_str(),"w");
      
      for(int imass=0;imass<nqmass;imass++)
	for(int jmass=0;jmass<nqmass;jmass++)
	  for(int r=0;r<nr;r++)
	    {
	      if(!pure_wilson) master_fprintf(fout," # m1(rev)=%lg m2(ins)=%lg r=%d\n",qmass[imass],qmass[jmass],r);
	      else             master_fprintf(fout," # kappa1(rev)=%lg kappa2(ins)=%lg\n",qkappa[imass],qkappa[jmass]);
	      print_contractions_to_file(fout,nhadr_contr,ig_hadr_so,ig_hadr_si,hadr_corr+ind*glb_size[0],0,"",1.0);
	      master_fprintf(fout,"\n");
	      ind+=nhadr_contr;
	    }
      
      //close the file
      close_file(fout);
    }
  
  print_time+=take_time();
}

//check if the time is enough
int check_remaining_time()
{
  int enough_time;
  
  //check remaining time
  double temp_time=take_time()+tot_prog_time;
  double ave_time=temp_time/nanalyzed_conf;
  double left_time=wall_time-temp_time;
  enough_time=left_time>(ave_time*1.1);
  
  master_printf("Remaining time: %lg sec\n",left_time);
  master_printf("Average time per conf: %lg sec, pessimistically: %lg\n",ave_time,ave_time*1.1);
  if(enough_time) master_printf("Continuing with next conf!\n");
  else master_printf("Not enough time, exiting!\n");
  
  return enough_time;
}

//close deallocating everything
void close()
{
  master_printf("\n");
  master_printf("Inverted %d configurations.\n",nanalyzed_conf);
  master_printf("Total time: %g, of which:\n",tot_prog_time);
  master_printf(" - %02.2f%s to prepare %d photon stochastic propagators (%2.2gs avg)\n",photon_prop_time/tot_prog_time*100,"%",nphoton_prop_tot,photon_prop_time/nphoton_prop_tot);
  master_printf(" - %02.2f%s to prepare %d lepton propagators (%2.2gs avg)\n",lepton_prop_time/tot_prog_time*100,"%",nlprop,lepton_prop_time/nlprop);
  master_printf(" - %02.2f%s to prepare %d generalized sources (%2.2gs avg)\n",source_time/tot_prog_time*100,"%",nsource_tot,source_time/nsource_tot);
  master_printf(" - %02.2f%s to perform %d inversions (%2.2gs avg)\n",inv_time/tot_prog_time*100,"%",ninv_tot,inv_time/ninv_tot);
  master_printf("    of which  %02.2f%s for %d cg inversion overhead (%2.2gs avg)\n",cg_inv_over_time/inv_time*100,"%",ninv_tot,cg_inv_over_time/ninv_tot);
  master_printf(" - %02.2f%s to perform %d hadronic contractions (%2.2gs avg)\n",hadr_contr_time/tot_prog_time*100,"%",nhadr_contr_tot,hadr_contr_time/nhadr_contr_tot);
  master_printf(" - %02.2f%s to perform %d leptonic contractions (%2.2gs avg)\n",lept_contr_time/tot_prog_time*100,"%",nlept_contr_tot,lept_contr_time/nlept_contr_tot);
  master_printf(" - %02.2f%s to print hadro-leptonic contractions\n",print_time/tot_prog_time*100,"%");
  
  for(int i=0;i<nphi_eta_alt;i++) nissa_free(photon_field[i]);
  nissa_free(source);
  nissa_free(original_source);
  for(int iprop=0;iprop<nqprop;iprop++) nissa_free(Q[iprop]);
  nissa_free(Q);
  for(int iprop=0;iprop<nlprop;iprop++) nissa_free(L[iprop]);
  nissa_free(L);
  nissa_free(temp_lep);
  nissa_free(conf);
  nissa_free(hadr_corr);
  nissa_free(glb_corr);
  nissa_free(loc_corr);
  nissa_free(hadr);
  nissa_free(hadrolept_corr);
  nissa_free(hadrolept_corr_chris);
  nissa_free(temp_source);
  nissa_free(temp_solution);
  nissa_free(lep_corr_iq1);
  nissa_free(lep_corr_iq2);
  nissa_free(leps);
  nissa_free(lep_energy);
  nissa_free(neu_energy);
}

void in_main(int narg,char **arg)
{
  //Basic mpi initialization
  tot_prog_time-=take_time();
  
  //check argument
  if(narg<2) crash("Use: %s input_file",arg[0]);
  
  //init simulation according to input file
  init_simulation(arg[1]);
  
  //test_photon_propagator();
  //test_muon_propagator(0);
  //test_muon_propagator(1);
  //test_muon_propagator(glb_size[3]-1);
  
  //loop over the configs
  int iconf=0,enough_time=1;
  while(iconf<ngauge_conf && enough_time && !file_exists("stop") && read_conf_parameters(iconf))
    {
      //setup the conf and generate the source
      start_new_conf();
      
      compute_lepton_free_loop();
      
      for(int isource=0;isource<nsources;isource++)
	{
	  //init
	  random_shift_gauge_conf(conf,old_theta,put_theta);
	  generate_photon_stochastic_propagator();
	  generate_original_source();
	  
	  if(chris_test) for(int t2=0;t2<glb_size[0];t2++) generate_lepton_propagators(t2);
	  generate_lepton_propagators(glb_size[0]);
	  generate_quark_propagators();
	  
	  compute_hadroleptonic_correlations();
	  compute_hadronic_correlations();
	  
	  //test for chris
	  if(chris_test)
	    for(int t1=0;t1<glb_size[0];t1++)
	      {
		generate_quark_propagators_chris(t1);
		for(int t2=0;t2<glb_size[0];t2++)
		  compute_hadroleptonic_correlations_chris(t1,t2);
	      }
	}
      
      //print out correlations
      print_correlations();
      
      //pass to the next conf if there is enough time
      char fin_file[1024];
      sprintf(fin_file,"%s/finished",outfolder);
      file_touch(fin_file);
      
      nanalyzed_conf++;
      enough_time=check_remaining_time();
    }
  
  //close the simulation
  tot_prog_time+=take_time();
  close();
}

int main(int narg,char **arg)
{
  init_nissa_threaded(narg,arg,in_main);
  close_nissa();
  
  return 0;
}
