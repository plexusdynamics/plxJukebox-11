

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>

#if defined STDC_HEADERS || defined _LIBC
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
# include <string.h>
#else
# include <strings.h>
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif
#if !HAVE_STRCHR && !defined _LIBC
# ifndef strchr
#  define strchr index
# endif
#endif

#if defined HAVE_UNISTD_H || defined _LIBC
# include <unistd.h>
#endif

#include "gettext.h"
#include "gettextP.h"
#ifdef _LIBC
# include <libintl.h>
#else
# include "libgettext.h"
#endif

/* @@ end of prolog @@ */
/* List of already loaded domains.  */
static struct loaded_l10nfile *_nl_loaded_domains;


/* Return a data structure describing the message catalog described by
   the DOMAINNAME and CATEGORY parameters with respect to the currently
   established bindings.  */
struct loaded_l10nfile *
internal_function
_nl_find_domain (dirname, locale, domainname)
     const char *dirname;
     char *locale;
     const char *domainname;
{
  struct loaded_l10nfile *retval;
  const char *language;
  const char *modifier;
  const char *territory;
  const char *codeset;
  const char *normalized_codeset;
  const char *special;
  const char *sponsor;
  const char *revision;
  const char *alias_value;
  int mask;

  /* LOCALE can consist of up to four recognized parts for the XPG syntax:

		language[_territory[.codeset]][@modifier]

     and six parts for the CEN syntax:

	language[_territory][+audience][+special][,[sponsor][_revision]]

     Beside the first part all of them are allowed to be missing.  If
     the full specified locale is not found, the less specific one are
     looked for.  The various parts will be stripped off according to
     the following order:
		(1) revision
		(2) sponsor
		(3) special
		(4) codeset
		(5) normalized codeset
		(6) territory
		(7) audience/modifier
   */

  /* If we have already tested for this locale entry there has to
     be one data set in the list of loaded domains.  */
  retval = _nl_make_l10nflist (&_nl_loaded_domains, dirname,
			       strlen (dirname) + 1, 0, locale, NULL, NULL,
			       NULL, NULL, NULL, NULL, NULL, domainname, 0);
  if (retval != NULL)
    {
      /* We know something about this locale.  */
      int cnt;

      if (retval->decided == 0)
	_nl_load_domain (retval);

      if (retval->data != NULL)
	return retval;

      for (cnt = 0; retval->successor[cnt] != NULL; ++cnt)
	{
	  if (retval->successor[cnt]->decided == 0)
	    _nl_load_domain (retval->successor[cnt]);

	  if (retval->successor[cnt]->data != NULL)
	    break;
	}
      return cnt >= 0 ? retval : NULL;
      /* NOTREACHED */
    }

  /* See whether the locale value is an alias.  If yes its value
     *overwrites* the alias name.  No test for the original value is
     done.  */
  alias_value = _nl_expand_alias (locale);
  if (alias_value != NULL)
    {
#if defined _LIBC || defined HAVE_STRDUP
      locale = strdup (alias_value);
      if (locale == NULL)
	return NULL;
#else
      size_t len = strlen (alias_value) + 1;
      locale = (char *) malloc (len);
      if (locale == NULL)
	return NULL;

      memcpy (locale, alias_value, len);
#endif
    }

  /* Now we determine the single parts of the locale name.  First
     look for the language.  Termination symbols are `_' and `@' if
     we use XPG4 style, and `_', `+', and `,' if we use CEN syntax.  */
  mask = _nl_explode_name (locale, &language, &modifier, &territory,
			   &codeset, &normalized_codeset, &special,
			   &sponsor, &revision);

  /* Create all possible locale entries which might be interested in
     generalization.  */
  retval = _nl_make_l10nflist (&_nl_loaded_domains, dirname,
			       strlen (dirname) + 1, mask, language, territory,
			       codeset, normalized_codeset, modifier, special,
			       sponsor, revision, domainname, 1);
  if (retval == NULL)
    /* This means we are out of core.  */
    return NULL;

  if (retval->decided == 0)
    _nl_load_domain (retval);
  if (retval->data == NULL)
    {
      int cnt;
      for (cnt = 0; retval->successor[cnt] != NULL; ++cnt)
	{
	  if (retval->successor[cnt]->decided == 0)
	    _nl_load_domain (retval->successor[cnt]);
	  if (retval->successor[cnt]->data != NULL)
	    break;
	}
    }

  /* The room for an alias was dynamically allocated.  Free it now.  */
  if (alias_value != NULL)
    free (locale);

  return retval;
}


#ifdef _LIBC
static void __attribute__ ((unused))
free_mem (void)
{
  struct loaded_l10nfile *runp = _nl_loaded_domains;

  while (runp != NULL)
    {
      struct loaded_l10nfile *here = runp;
      if (runp->data != NULL)
	_nl_unload_domain ((struct loaded_domain *) runp->data);
      runp = runp->next;
      free (here);
    }
}

text_set_element (__libc_subfreeres, free_mem);
#endif
