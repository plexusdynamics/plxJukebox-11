

/* Avoid using the symbol _ALLOCA_H here, as Bison assumes _ALLOCA_H
   means there is a real alloca function.  */
#ifndef _GNULIB_ALLOCA_H
# define _GNULIB_ALLOCA_H

/* alloca (N) returns a pointer to N bytes of memory
   allocated on the stack, which will last until the function returns.
   Use of alloca should be avoided:
     - inside arguments of function calls - undefined behaviour,
     - in inline functions - the allocation may actually last until the
       calling function returns,
     - for huge N (say, N >= 65536) - you never know how large (or small)
       the stack is, and when the stack cannot fulfill the memory allocation
       request, the program just crashes.
 */

#ifndef alloca
# ifdef __GNUC__
#  define alloca __builtin_alloca
# elif defined _AIX
#  define alloca __alloca
# elif defined _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# else
#  include <stddef.h>
#  ifdef  __cplusplus
extern "C"
#  endif
void *alloca (size_t);
# endif
#endif

#endif 
