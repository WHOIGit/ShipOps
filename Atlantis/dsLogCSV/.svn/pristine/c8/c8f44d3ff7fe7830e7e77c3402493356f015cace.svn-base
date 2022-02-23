/* ======================================================================
   Useful macros

   Modification History
   Date      Author  Comment
   ----------------------------------------------------------------------
   12-20-92  LLW     Created and Writen
   02-18-02  LLW     Added more general macros and #defines
   
   ====================================================================== */

#ifndef JASON_MACRO_INCLUDE
#define JASON_MACRO_INCLUDE

// 19 Feb 2002 LLW Moved from dsLogTalk.h
#define TRUE    1
#define FALSE   0

#define fsign(x)  ((x)>0.0 ? 1.0 : ( (x)<0 ? -1.0 : 0.0 ) )
#define fssqrt(x) (fsign((x)) * sqrt(fabs(x)))


#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#define inrange(val,min,max) ( ((val) >= (min)) && ((val) <= (max)))
#define fsat(x,max) ((x)>fabs(max))? fabs(max) : (((x)<(-fabs(max)))? (-fabs(max)): x)


#endif
