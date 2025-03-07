

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "printf-args.h"

#ifdef STATIC
STATIC
#endif
int
printf_fetchargs (va_list args, arguments *a)
{
  size_t i;
  argument *ap;

  for (i = 0, ap = &a->arg[0]; i < a->count; i++, ap++)
    switch (ap->type)
      {
      case TYPE_SCHAR:
	ap->a.a_schar = va_arg (args, /*signed char*/ int);
	break;
      case TYPE_UCHAR:
	ap->a.a_uchar = va_arg (args, /*unsigned char*/ int);
	break;
      case TYPE_SHORT:
	ap->a.a_short = va_arg (args, /*short*/ int);
	break;
      case TYPE_USHORT:
	ap->a.a_ushort = va_arg (args, /*unsigned short*/ int);
	break;
      case TYPE_INT:
	ap->a.a_int = va_arg (args, int);
	break;
      case TYPE_UINT:
	ap->a.a_uint = va_arg (args, unsigned int);
	break;
      case TYPE_LONGINT:
	ap->a.a_longint = va_arg (args, long int);
	break;
      case TYPE_ULONGINT:
	ap->a.a_ulongint = va_arg (args, unsigned long int);
	break;
#ifdef HAVE_LONG_LONG
      case TYPE_LONGLONGINT:
	ap->a.a_longlongint = va_arg (args, long long int);
	break;
      case TYPE_ULONGLONGINT:
	ap->a.a_ulonglongint = va_arg (args, unsigned long long int);
	break;
#endif
      case TYPE_DOUBLE:
	ap->a.a_double = va_arg (args, double);
	break;
#ifdef HAVE_LONG_DOUBLE
      case TYPE_LONGDOUBLE:
	ap->a.a_longdouble = va_arg (args, long double);
	break;
#endif
      case TYPE_CHAR:
	ap->a.a_char = va_arg (args, int);
	break;
#ifdef HAVE_WINT_T
      case TYPE_WIDE_CHAR:
	ap->a.a_wide_char = va_arg (args, wint_t);
	break;
#endif
      case TYPE_STRING:
	ap->a.a_string = va_arg (args, const char *);
	/* A null pointer is an invalid argument for "%s", but in practice
	   it occurs quite frequently in printf statements that produce
	   debug output.  Use a fallback in this case.  */
	if (ap->a.a_string == NULL)
	  ap->a.a_string = "(NULL)";
	break;
#ifdef HAVE_WCHAR_T
      case TYPE_WIDE_STRING:
	ap->a.a_wide_string = va_arg (args, const wchar_t *);
	/* A null pointer is an invalid argument for "%ls", but in practice
	   it occurs quite frequently in printf statements that produce
	   debug output.  Use a fallback in this case.  */
	if (ap->a.a_wide_string == NULL)
	  {
	    static const wchar_t wide_null_string[] =
	      {
		(wchar_t)'(',
		(wchar_t)'N', (wchar_t)'U', (wchar_t)'L', (wchar_t)'L',
		(wchar_t)')',
		(wchar_t)0
	      };
	    ap->a.a_wide_string = wide_null_string;
	  }
	break;
#endif
      case TYPE_POINTER:
	ap->a.a_pointer = va_arg (args, void *);
	break;
      case TYPE_COUNT_SCHAR_POINTER:
	ap->a.a_count_schar_pointer = va_arg (args, signed char *);
	break;
      case TYPE_COUNT_SHORT_POINTER:
	ap->a.a_count_short_pointer = va_arg (args, short *);
	break;
      case TYPE_COUNT_INT_POINTER:
	ap->a.a_count_int_pointer = va_arg (args, int *);
	break;
      case TYPE_COUNT_LONGINT_POINTER:
	ap->a.a_count_longint_pointer = va_arg (args, long int *);
	break;
#ifdef HAVE_LONG_LONG
      case TYPE_COUNT_LONGLONGINT_POINTER:
	ap->a.a_count_longlongint_pointer = va_arg (args, long long int *);
	break;
#endif
      default:
	/* Unknown type.  */
	return -1;
      }
  return 0;
}
