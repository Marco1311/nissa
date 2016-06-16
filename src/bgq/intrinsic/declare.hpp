#ifndef _BGQ_INTRINSIC_DECLARE_HPP
#define _BGQ_INTRINSIC_DECLARE_HPP

#define DECLARE_REG_VIR_COLOR(A)		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c0));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c1));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c2))

#define DECLARE_REG_VIR_SPINCOLOR(A)		\
  DECLARE_REG_VIR_COLOR(NAME2(A,s0));		\
  DECLARE_REG_VIR_COLOR(NAME2(A,s1));		\
  DECLARE_REG_VIR_COLOR(NAME2(A,s2));		\
  DECLARE_REG_VIR_COLOR(NAME2(A,s3))

#define DECLARE_REG_VIR_HALFSPIN(A)		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,s0));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,s1))

#define DECLARE_REG_VIR_HALFSPINCOLOR(A)	\
  DECLARE_REG_VIR_COLOR(NAME2(A,s0));		\
  DECLARE_REG_VIR_COLOR(NAME2(A,s1))

#define DECLARE_REG_VIR_PARTIAL_SU3(A)			\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c00));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c01));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c02));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c10));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c11));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c12))

#define DECLARE_REG_VIR_SU3(A)				\
  DECLARE_REG_VIR_PARTIAL_SU3(A);			\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c20));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c21));		\
  DECLARE_REG_VIR_COMPLEX(NAME2(A,c22))

#endif
