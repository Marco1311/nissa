#ifndef _OPENMP_MACROS_H
#define _OPENMP_MACROS_H

#include "debug.h"

//////////////////////////////////////////////////////////////////////////////////////

#define IS_MASTER_THREAD (thread_id==0)

#define NISSA_PARALLEL_LOOP(INDEX,WORKLOAD)                             \
  if(WORKLOAD%nthreads)                                                 \
    crash("workload %d not multiple of thread number %d",WORKLOAD,nthreads); \
  for(int INDEX=thread_id*(WORKLOAD/nthreads);INDEX<(thread_id+1)*(WORKLOAD/nthreads);INDEX++)

//////////////////////////////////////////////////////////////////////////////////////

//external argument to exchange info between function and worker
#define EXTERNAL_ARG(FUNC_NAME,ARG_TYPE,ARG) ARG_TYPE NAME2(FUNC_NAME,ARG);
#define EXPORT_ARG(FUNC_NAME,ARG) NAME2(FUNC_NAME,ARG)=ARG;
#define IMPORT_ARG(FUNC_NAME,ARG_TYPE,ARG) ARG_TYPE ARG=NAME2(FUNC_NAME,ARG);

//////////////////////////////////////////////////////////////////////////////////////

//headers: external parameters and prototype
#define THREADABLE_FUNCTION_0ARG_HEADER(FUNC_NAME) void NAME2(FUNC_NAME,WORKER)();
#define THREADABLE_FUNCTION_1ARG_HEADER(FUNC_NAME,AT1,A1)		\
  EXTERNAL_ARG(FUNC_NAME,AT1,A1)					\
  THREADABLE_FUNCTION_0ARG_HEADER(FUNC_NAME)
#define THREADABLE_FUNCTION_2ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2)	\
  EXTERNAL_ARG(FUNC_NAME,AT2,A2)					\
  THREADABLE_FUNCTION_1ARG_HEADER(FUNC_NAME,AT1,A1)
#define THREADABLE_FUNCTION_3ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  EXTERNAL_ARG(FUNC_NAME,AT3,A3)					\
  THREADABLE_FUNCTION_2ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2)
#define THREADABLE_FUNCTION_4ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  EXTERNAL_ARG(FUNC_NAME,AT4,A4)					\
  THREADABLE_FUNCTION_3ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)
#define THREADABLE_FUNCTION_5ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  EXTERNAL_ARG(FUNC_NAME,AT5,A5)					\
  THREADABLE_FUNCTION_4ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4)
#define THREADABLE_FUNCTION_6ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  EXTERNAL_ARG(FUNC_NAME,AT6,A6)					\
  THREADABLE_FUNCTION_5ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5)
#define THREADABLE_FUNCTION_7ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  EXTERNAL_ARG(FUNC_NAME,AT7,A7)					\
  THREADABLE_FUNCTION_6ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6)
#define THREADABLE_FUNCTION_8ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  EXTERNAL_ARG(FUNC_NAME,AT8,A8)					\
  THREADABLE_FUNCTION_7ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7)
#define THREADABLE_FUNCTION_9ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  EXTERNAL_ARG(FUNC_NAME,AT9,A9)					\
  THREADABLE_FUNCTION_8ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8)
#define THREADABLE_FUNCTION_10ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  EXTERNAL_ARG(FUNC_NAME,AT10,A10)					\
  THREADABLE_FUNCTION_9ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9)

///////////////////////////////////////////////////////////////////////////////////////

//external function: exportation (last line is most external)
#define THREADABLE_FUNCTION_0ARG_EXPORT(FUNC_NAME) {
#define THREADABLE_FUNCTION_1ARG_EXPORT(FUNC_NAME,AT1,A1)		\
  THREADABLE_FUNCTION_0ARG_EXPORT(FUNC_NAME)				\
  EXPORT_ARG(FUNC_NAME,A1)
#define THREADABLE_FUNCTION_2ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2)	\
  THREADABLE_FUNCTION_1ARG_EXPORT(FUNC_NAME,AT1,A1)			\
  EXPORT_ARG(FUNC_NAME,A2)
#define THREADABLE_FUNCTION_3ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  THREADABLE_FUNCTION_2ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2)		\
  EXPORT_ARG(FUNC_NAME,A3)
#define THREADABLE_FUNCTION_4ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  THREADABLE_FUNCTION_3ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  EXPORT_ARG(FUNC_NAME,A4)
#define THREADABLE_FUNCTION_5ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_4ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  EXPORT_ARG(FUNC_NAME,A5)
#define THREADABLE_FUNCTION_6ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_5ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  EXPORT_ARG(FUNC_NAME,A6)
#define THREADABLE_FUNCTION_7ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_6ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  EXPORT_ARG(FUNC_NAME,A7)
#define THREADABLE_FUNCTION_8ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_7ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  EXPORT_ARG(FUNC_NAME,A8)
#define THREADABLE_FUNCTION_9ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_8ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  EXPORT_ARG(FUNC_NAME,A9)
#define THREADABLE_FUNCTION_10ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_9ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  EXPORT_ARG(FUNC_NAME,A10)

/////////////////////////////////////////////////////////////////////////////////////////

//body: issue worker and reimport (last line is again the most external, so this is why we have to split)
#define THREADABLE_FUNCTION_0ARG_BODY(FUNC_NAME)			\
  if(thread_pool_locked&&IS_MASTER_THREAD) start_threaded_function(NAME2(FUNC_NAME,WORKER)); \
  else NAME2(FUNC_NAME,WORKER)();					\
 }									\
									\
  void NAME2(FUNC_NAME,WORKER)()					\
  {
#define THREADABLE_FUNCTION_1ARG_BODY(FUNC_NAME,AT1,A1)	\
  THREADABLE_FUNCTION_0ARG_BODY(FUNC_NAME)		\
  IMPORT_ARG(FUNC_NAME,AT1,A1)
#define THREADABLE_FUNCTION_2ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2)	\
  THREADABLE_FUNCTION_1ARG_BODY(FUNC_NAME,AT1,A1)		\
  IMPORT_ARG(FUNC_NAME,AT2,A2)
#define THREADABLE_FUNCTION_3ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  THREADABLE_FUNCTION_2ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2)		\
  IMPORT_ARG(FUNC_NAME,AT3,A3)
#define THREADABLE_FUNCTION_4ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  THREADABLE_FUNCTION_3ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)		\
  IMPORT_ARG(FUNC_NAME,AT4,A4)
#define THREADABLE_FUNCTION_5ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_4ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4)	\
  IMPORT_ARG(FUNC_NAME,AT5,A5)
#define THREADABLE_FUNCTION_6ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_5ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  IMPORT_ARG(FUNC_NAME,AT6,A6)
#define THREADABLE_FUNCTION_7ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_6ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  IMPORT_ARG(FUNC_NAME,AT7,A7)
#define THREADABLE_FUNCTION_8ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_7ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  IMPORT_ARG(FUNC_NAME,AT8,A8)
#define THREADABLE_FUNCTION_9ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_8ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  IMPORT_ARG(FUNC_NAME,AT9,A9)
#define THREADABLE_FUNCTION_10ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_9ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  IMPORT_ARG(FUNC_NAME,AT10,A10)

//////////////////////////////////////////////////////////////////////////////////////////

//threadable function with 0 arguments
#define THREADABLE_FUNCTION_0ARG(FUNC_NAME)				\
  THREADABLE_FUNCTION_0ARG_HEADER(FUNC_NAME)				\
  void FUNC_NAME()							\
  THREADABLE_FUNCTION_0ARG_EXPORT(FUNC_NAME)				\
  THREADABLE_FUNCTION_0ARG_BODY(FUNC_NAME)

//threadable function with 1 arguments
#define THREADABLE_FUNCTION_1ARG(FUNC_NAME,AT1,A1)			\
  THREADABLE_FUNCTION_1ARG_HEADER(FUNC_NAME,AT1,A1)			\
  void FUNC_NAME(AT1 A1)						\
  THREADABLE_FUNCTION_1ARG_EXPORT(FUNC_NAME,AT1,A1)			\
  THREADABLE_FUNCTION_1ARG_BODY(FUNC_NAME,AT1,A1)

//threadable function with 2 arguments
#define THREADABLE_FUNCTION_2ARG(FUNC_NAME,AT1,A1,AT2,A2)		\
  THREADABLE_FUNCTION_2ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2)		\
  void FUNC_NAME(AT1 A1,AT2 A2)						\
  THREADABLE_FUNCTION_2ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2)		\
  THREADABLE_FUNCTION_2ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2)

//threadable function with 3 arguments
#define THREADABLE_FUNCTION_3ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  THREADABLE_FUNCTION_3ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3)					\
  THREADABLE_FUNCTION_3ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  THREADABLE_FUNCTION_3ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)

//threadable function with 4 arguments
#define THREADABLE_FUNCTION_4ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4)	\
  THREADABLE_FUNCTION_4ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4)				\
  THREADABLE_FUNCTION_4ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  THREADABLE_FUNCTION_4ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4)

//threadable function with 5 arguments
#define THREADABLE_FUNCTION_5ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_5ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5)			\
  THREADABLE_FUNCTION_5ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_5ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5)

//threadable function with 6 arguments
#define THREADABLE_FUNCTION_6ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_6ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6)			\
  THREADABLE_FUNCTION_6ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_6ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6)

//threadable function with 7 arguments
#define THREADABLE_FUNCTION_7ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_7ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7)		\
  THREADABLE_FUNCTION_7ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_7ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7)

//threadable function with 8 arguments
#define THREADABLE_FUNCTION_8ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_8ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8)	\
  THREADABLE_FUNCTION_8ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_8ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8)

//threadable function with 9 arguments
#define THREADABLE_FUNCTION_9ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_9ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9) \
  THREADABLE_FUNCTION_9ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_9ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9)

//threadable function with 10 arguments
#define THREADABLE_FUNCTION_10ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_10ARG_HEADER(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9,AT10 A10) \
  THREADABLE_FUNCTION_10ARG_EXPORT(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_10ARG_BODY(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10)

#endif
