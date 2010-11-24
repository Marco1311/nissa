#pragma once

#include <iostream>
#include <math.h>

#include "global.cpp"

using namespace std;

//The structure for gamma matrix
struct dirac_matr
{
public:
  int pos[4];
  complex entr[4];
};

//The base of the 16 gamma matrixes and the two rotators
dirac_matr base_gamma[16];
dirac_matr Pplus,Pminus;

//Initialize a dirac matrix with outside entries
void init_dirac(dirac_matr &out,
		int pos0,double rea0,double ima0,
		int pos1,double rea1,double ima1,
		int pos2,double rea2,double ima2,
		int pos3,double rea3,double ima3)
{
  out.pos[0]=pos0;
  out.pos[1]=pos1;
  out.pos[2]=pos2;
  out.pos[3]=pos3;

  out.entr[0][0]=rea0;
  out.entr[1][0]=rea1;
  out.entr[2][0]=rea2;
  out.entr[3][0]=rea3;

  out.entr[0][1]=ima0;
  out.entr[1][1]=ima1;
  out.entr[2][1]=ima2;
  out.entr[3][1]=ima3;
}

//If the two dirac matrix in1 and in2 have the same position structure sum
//them, otherwise it crashes
void dirac_summ(dirac_matr &out,dirac_matr &in1,dirac_matr &in2)
{
  for(int ig=0;ig<4;ig++)
    if(in1.pos[ig]==in2.pos[ig])
      {
	out.pos[ig]=in1.pos[ig];
	complex_summ(out.entr[ig],in1.entr[ig],in2.entr[ig]);
      }
    else 
      {
	if(rank==0) cerr<<"The two matrix passed to sum have different positions"<<endl;
	MPI_Abort(MPI_COMM_WORLD,1);
      }
}

//Assign to the first dirac matrixes the product of the second and the third
void dirac_prod(dirac_matr &out,dirac_matr &in1,dirac_matr &in2)
{
  dirac_matr temp; //this is needed to avoid to overwrite one of the input

  //This is the line on the first matrix
  for(int ig1=0;ig1<4;ig1++)
    {
      //This is the line to be taken on the second matrix
      int ig2=in1.pos[ig1];

      //For each line, the column of the output matrix which is
      //different from 0 is the column of the second matrix different
      //from 0 on the line with index equal to the column of the first
      //matrix which is different from 0 (that is, ig2)
      temp.pos[ig1]=in2.pos[ig2];
      
      //The entries of the output is, on each line, the complex
      //product of the entries of the first matrix on that line, for
      //the entries of the second matrix on the line with the index
      //equal to the column of the first matrix which is different
      //from 0 (which again is ig2)
      complex_prod(temp.entr[ig1],in1.entr[ig1],in2.entr[ig2]);
    }
  
  memcpy(out.pos,temp.pos,sizeof(int)*4);
  memcpy(out.entr,temp.entr,sizeof(complex)*4);  
}

//Assign to the first dirac the product of the second by the complex
//number passed as argument
void dirac_compl_prod(dirac_matr &out,dirac_matr &in,complex c)
{
  //This is the line on the matrix
  for(int ig=0;ig<4;ig++)
    {
      out.pos[ig]=in.pos[ig];

      complex_prod(out.entr[ig],in.entr[ig],c);
    }
}

//Assign to the first spinspin the product of the passed dirac matrix by the passed spinspin
void spinspin_dirac_spinspin_prod(spinspin &out,dirac_matr &m,spinspin &in)
{
  //This is the line on the matrix
  for(int id1=0;id1<4;id1++)
    for(int id2=0;id2<4;id2++)
      complex_prod(out[id1][id2],m.entr[id1],in[m.pos[id1]][id2]);
}

//Assign to the first spinspin the product of the passed dirac matrix by the passed spinspin
void spinspin_dirac_spinspindag_prod(spinspin &out,dirac_matr &m,spinspin &in)
{
  //This is the line on the matrix
  for(int id1=0;id1<4;id1++)
    for(int id2=0;id2<4;id2++)
      complex_conj_prod(out[id1][id2],m.entr[id1],in[id2][m.pos[id1]]);
}

//Print the dirac marix passed as argument only on node 0
void print_dirac(dirac_matr &in)
{
  for(int ir=0;ir<4;ir++)
    {
      int pos=in.pos[ir];
      for(int ic=0;ic<pos;ic++) cout<<"0,0\t";
      cout<<in.entr[ir][0]<<","<<in.entr[ir][1]<<"\t";
      for(int ic=pos+1;ic<4;ic++) cout<<"0,0\t";
      cout<<endl;
    }
}

//Initialize the gamma matrix base and the rotators
// base_gamma[ 0] = identity
// base_gamma[ 1] = gamma_1
// base_gamma[ 2] = gamma_2
// base_gamma[ 3] = gamma_3
// base_gamma[ 4] = gamma_0
// base_gamma[ 5] = gamma_5
// base_gamma[ 6] = gamma_15
// base_gamma[ 7] = gamma_25
// base_gamma[ 8] = gamma_35
// base_gamma[ 9] = gamma_05
// base_gamma[10] = gamma_01
// base_gamma[11] = gamma_02
// base_gamma[12] = gamma_03
// base_gamma[13] = gamma_23
// base_gamma[14] = gamma_31
// base_gamma[15] = gamma_12

void init_base_gamma()
{
  const double rad2=1./sqrt(2);

  init_dirac(base_gamma[ 0],  0,1,0  , 1,1,0  , 2,1,0  , 3,1,0 );
  init_dirac(base_gamma[ 1],  3,0,-1 , 2,0,-1 , 1,0,1  , 0,0,1 );
  init_dirac(base_gamma[ 2],  3,-1,0 , 2,1,0  , 1,1,0  , 0,-1,0);
  init_dirac(base_gamma[ 3],  2,0,-1 , 3,0,1  , 0,0,1  , 1,0,-1);
  init_dirac(base_gamma[ 4],  2,-1,0 , 3,-1,0 , 0,-1,0 , 1,-1,0);
  init_dirac(base_gamma[ 5],  0,1,0  , 1,1,0  , 2,-1,0 , 3,-1,0);
  init_dirac(base_gamma[ 6],  3,0,1  , 2,0,1  , 1,0,1  , 0,0,1 );
  init_dirac(base_gamma[ 7],  3,1,0  , 2,-1,0 , 1,1,0  , 0,-1,0);
  init_dirac(base_gamma[ 8],  2,0,1  , 3,0,-1 , 0,0,1  , 1,0,-1);
  init_dirac(base_gamma[ 9],  2,1,0  , 3,1,0  , 0,-1,0 , 1,-1,0);
  init_dirac(base_gamma[10],  1,0,-1 , 0,0,-1 , 3,0,1  , 2,0,1 );
  init_dirac(base_gamma[11],  1,-1,0 , 0,1,0  , 3,1,0  , 2,-1,0);
  init_dirac(base_gamma[12],  0,0,-1 , 1,0,1  , 2,0,1  , 3,0,-1);
  init_dirac(base_gamma[13],  1,0,1  , 0,0,1  , 3,0,1  , 2,0,1 );
  init_dirac(base_gamma[14],  1,1,0  , 0,-1,0 , 3,1,0  , 2,-1,0);
  init_dirac(base_gamma[15],  0,0,1  , 1,0,-1 , 2,0,1  , 3,0,-1);

  init_dirac(Pplus ,          0,rad2,rad2  , 1,rad2,rad2  , 2,rad2,-rad2 , 3,rad2,-rad2);
  init_dirac(Pminus,          0,rad2,-rad2 , 1,rad2,-rad2 , 2,rad2,rad2  , 3,rad2,rad2 );

}

//Trace of the product off two spinspins
void trace_prod_spinspins(complex c,spinspin &a,spinspin &b)
{
  c[0]=c[1]=0;
  for(int id1=0;id1<4;id1++)
    for(int id2=0;id2<4;id2++)
      complex_summ_the_prod(c,a[id1][id2],b[id2][id1]);
}
