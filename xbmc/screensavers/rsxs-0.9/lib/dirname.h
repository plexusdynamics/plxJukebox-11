

#ifndef DIRNAME_H_
# define DIRNAME_H_ 1

# include <stdbool.h>
# include <stddef.h>

# ifndef DIRECTORY_SEPARATOR
#  define DIRECTORY_SEPARATOR '/'
# endif

# ifndef ISSLASH
#  define ISSLASH(C) ((C) == DIRECTORY_SEPARATOR)
# endif

# ifndef FILE_SYSTEM_PREFIX_LEN
#  if FILE_SYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX
    /* This internal macro assumes ASCII, but all hosts that support drive
       letters use ASCII.  */
#   define _IS_DRIVE_LETTER(c) (((unsigned int) (c) | ('a' - 'A')) - 'a' \
				<= 'z' - 'a')
#   define FILE_SYSTEM_PREFIX_LEN(Filename) \
	   (_IS_DRIVE_LETTER ((Filename)[0]) && (Filename)[1] == ':' ? 2 : 0)
#  else
#   define FILE_SYSTEM_PREFIX_LEN(Filename) 0
#  endif
# endif

# ifndef FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE
#  define FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE 0
# endif

# ifndef DOUBLE_SLASH_IS_DISTINCT_ROOT
#  define DOUBLE_SLASH_IS_DISTINCT_ROOT 1
# endif

# if FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE
#  define IS_ABSOLUTE_FILE_NAME(F) ISSLASH ((F)[FILE_SYSTEM_PREFIX_LEN (F)])
# else
#  define IS_ABSOLUTE_FILE_NAME(F) \
	  (ISSLASH ((F)[0]) || 0 < FILE_SYSTEM_PREFIX_LEN (F))
# endif
# define IS_RELATIVE_FILE_NAME(F) (! IS_ABSOLUTE_FILE_NAME (F))

char *base_name (char const *file);
char *dir_name (char const *file);
size_t base_len (char const *file);
size_t dir_len (char const *file);
char *last_component (char const *file);

bool strip_trailing_slashes (char *file);

#endif /* not DIRNAME_H_ */
