


#ifndef _EMU_MSVCRT_H_
#define _EMU_MSVCRT_H_

#ifdef _LINUX
#define _onexit_t void*
#endif

#ifdef WIN32
#include "win32-dirent.h"
#else
#include <dirent.h>
#endif

typedef void ( *PFV)(void);

#define __IS_STDIN_STREAM(stream)   (stream == stdin  || stream->_file == stdin->_file || stream->_file == 0)
#define __IS_STDOUT_STREAM(stream)  (stream == stdout || stream->_file == stdout->_file || stream->_file == 1)
#define __IS_STDERR_STREAM(stream)  (stream == stderr || stream->_file == stderr->_file || stream->_file == 2)
#define IS_STDIN_STREAM(stream)     (stream != NULL && __IS_STDIN_STREAM(stream))
#define IS_STDOUT_STREAM(stream)    (stream != NULL && __IS_STDOUT_STREAM(stream))
#define IS_STDERR_STREAM(stream)    (stream != NULL && __IS_STDERR_STREAM(stream))
#ifdef _WIN32
#define IS_VALID_STREAM(stream)     (stream != NULL && (stream->_ptr != NULL))
#else
#define IS_VALID_STREAM(stream)     true
#endif


#define IS_STD_STREAM(stream)       (stream != NULL && (__IS_STDIN_STREAM(stream) || __IS_STDOUT_STREAM(stream) || __IS_STDERR_STREAM(stream)))

#define IS_STDIN_DESCRIPTOR(fd)  (fd == 0)
#define IS_STDOUT_DESCRIPTOR(fd) (fd == 1)
#define IS_STDERR_DESCRIPTOR(fd) (fd == 2)

#define IS_STD_DESCRIPTOR(fd) (IS_STDIN_DESCRIPTOR(fd) || IS_STDOUT_DESCRIPTOR(fd) || IS_STDERR_DESCRIPTOR(fd))


extern "C"
{
  char* dll_strdup( const char* str);
  void dll_sleep(unsigned long imSec);
  void InitFiles();
  void dllReleaseAll( );
  void* dllmalloc(size_t size);
  void dllfree( void* pPtr );
  void* dllcalloc( size_t num, size_t size );
  void* dllrealloc( void *memblock, size_t size );
  void dllexit(int iCode);
  void dllabort();
  void* dll__dllonexit(PFV input, PFV ** start, PFV ** end);
  _onexit_t dll_onexit(_onexit_t func);
  int dllputs(const char* szLine);
  int dll_putchar(int c);
  int dll_putc(int c, FILE *stream);
  int dllprintf( const char *format, ... );
  int dllvprintf(const char *format, va_list va);
  char *dll_fullpath(char *absPath, const char *relPath, size_t maxLength);
  FILE *dll_popen(const char *command, const char *mode);
  int dll_pclose(FILE *stream);
  FILE* dll_fdopen(int i, const char* file);
  int dll_open(const char* szFileName, int iMode);
  int dll_read(int fd, void* buffer, unsigned int uiSize);
  int dll_write(int fd, const void* buffer, unsigned int uiSize);
  int dll_close(int fd);
  __off64_t dll_lseeki64(int fd, __off64_t lPos, int iWhence);
  __off_t dll_lseek(int fd, __off_t lPos, int iWhence);
  char* dll_getenv(const char* szKey);
  int dll_fclose (FILE * stream);
#ifndef _LINUX
  intptr_t dll_findfirst(const char *file, struct _finddata_t *data);
  int dll_findnext(intptr_t f, _finddata_t* data);
  int dll_findclose(intptr_t handle);
  intptr_t dll_findfirst64i32(const char *file, struct _finddata64i32_t *data);
  int dll_findnext64i32(intptr_t f, _finddata64i32_t* data);
  void dll__security_error_handler(int code, void *data);
#endif
  DIR *dll_opendir(const char *filename);
  struct dirent *dll_readdir(DIR *dirp);
  int dll_closedir(DIR *dirp);
  void dll_rewinddir(DIR *dirp);
  char * dll_fgets (char* pszString, int num , FILE * stream);
  int dll_fgetc (FILE* stream);
  int dll_feof (FILE * stream);
  int dll_fread (void * buffer, size_t size, size_t count, FILE * stream);
  int dll_getc (FILE * stream);
  FILE * dll_fopen (const char * filename, const char * mode);
  int dll_fputc (int character, FILE * stream);
  int dll_putcchar (int character);
  int dll_fputs (const char * szLine , FILE* stream);
  int dll_fseek ( FILE * stream , long offset , int origin );
  int dll_fseek64(FILE *stream, off64_t offset, int origin);
  int dll_ungetc (int c, FILE * stream);
  long dll_ftell(FILE *stream);
  off64_t dll_ftell64(FILE *stream);
  long dll_tell ( int fd );
  __int64 dll_telli64 ( int fd );
  size_t dll_fwrite ( const void * buffer, size_t size, size_t count, FILE * stream );
  int dll_fflush (FILE * stream);
  int dll_ferror (FILE * stream);
  int dll_vfprintf(FILE *stream, const char *format, va_list va);
  int dll_fprintf(FILE* stream , const char * format, ...);
  int dll_fgetpos(FILE* stream, fpos_t* pos);
  int dll_fgetpos64(FILE *stream, fpos64_t *pos);
  int dll_fsetpos(FILE* stream, const fpos_t* pos);
  int dll_fsetpos64(FILE* stream, const fpos64_t* pos);
  int dll_fileno(FILE* stream);
  void dll_rewind(FILE* stream);
  void dll_clearerr(FILE* stream);
  int dll_initterm(PFV * start, PFV * end);
  uintptr_t dll_beginthread(void( *start_address )( void * ),unsigned stack_size,void *arglist);
  HANDLE dll_beginthreadex(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
                           LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags,
                           LPDWORD lpThreadId);
  int dll_stati64(const char *path, struct _stati64 *buffer);
  int dll_stat64(const char *path, struct __stat64 *buffer);
#ifdef _WIN32
  int dll_stat64i32(const char *path, struct _stat64i32 *buffer);
#endif
  int dll_stat(const char *path, struct stat *buffer);
  int dll_fstat(int fd, struct stat *buffer);
  int dll_fstati64(int fd, struct _stati64 *buffer);
  int dll_setmode(int handle, int mode );
  void dllperror(const char* s);
  char* dllstrerror(int iErr);
  int dll_mkdir(const char* dir);
  char* dll_getcwd(char *buffer, int maxlen);
  int dll_putenv(const char* envstring);
  int dll_ctype(int i);
  int dll_system(const char *command);
  void (__cdecl * dll_signal(int sig, void (__cdecl *func)(int)))(int);
  int dll_getpid();
  int dll__commit(int fd);
  char*** dll___p__environ();
  FILE* dll_freopen(const char *path, const char *mode, FILE *stream);
  int dll_fscanf(FILE *stream, const char *format , ...);
  void dll_flockfile(FILE *file);
  int dll_ftrylockfile(FILE *file);
  void dll_funlockfile(FILE *file);
  int dll_fstat64(int fd, struct __stat64 *buf);
#ifdef _WIN32
  int dll_fstat64i32(int fd, struct _stat64i32 *buffer);
  int dll_open_osfhandle(intptr_t _OSFileHandle, int _Flags);
#endif
  int dll_fstatvfs64(int fd, struct statvfs64 *buf);
  int dll_setvbuf(FILE *stream, char *buf, int type, size_t size);
  int dll_filbuf(FILE *fp);
  int dll_flsbuf(int data, FILE*fp);

#ifdef _LINUX
  int * __cdecl dll_errno(void);
#endif

  extern char **dll__environ;
}



#endif // _EMU_MSVCRT_H_

