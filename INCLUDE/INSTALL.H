#ifndef INSTALLER_H
#define INSTALLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Error codes ---- */
#define INS_OK           0
#define INS_ECANTOPENW  -1
#define INS_ECANTOPENR  -2
#define INS_ENOTFOUND   -3
#define INS_EBADFORMAT  -4
#define INS_ENOMEM      -5
#define INS_EINVAL      -6
#define INS_EABORTED    -7

/* ---- snprintf compat ---- */
#if defined(_MSC_VER) && _MSC_VER < 1900
# define insSnprintf  _snprintf
# define insVsnprintf _vsnprintf
#elif defined(__MSDOS__) || defined(__BORLANDC__)
int insSnprintf(char *buf, size_t n, const char *fmt, ...);
int insVsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
#else
# define insSnprintf  snprintf
# define insVsnprintf vsnprintf
#endif

/* ---- Fixed-width types for archive format ---- */
typedef unsigned short InsU16;
typedef unsigned long  InsU32;

/* ---- Cross-platform directory/path helpers ---- */
#if defined(__MSDOS__) || defined(__BORLANDC__)
# define INS_MKDIR(p)  mkdir(p)
#elif defined(_WIN32)
# define INS_MKDIR(p)  _mkdir(p)
#else
# define INS_MKDIR(p)  mkdir(p, 0755)
#endif
#if defined(__MSDOS__) || defined(__BORLANDC__)
# include <direct.h>
# include <dos.h>
# define chDir(p)      chdir(p)
# define mkDir(p)      mkdir(p)
# define getCwd(p,n)   getcwd(p,n)
# define fullPath(d,s,z) (strcpy(d,s),d)
#elif defined(_WIN32)
# include <direct.h>
# include <io.h>
# define chDir(p)      _chdir(p)
# define mkDir(p)      _mkdir(p)
# define getCwd(p,n)   _getcwd(p,n)
# define fullPath(d,s,z) _fullpath(d,s,z)
#else
# include <unistd.h>
# include <dirent.h>
# define chDir(p)      chdir(p)
# define mkDir(p)      mkdir(p, 0755)
# define getCwd(p,n)   getcwd(p,n)
# define fullPath(d,s,z) (realpath(s,d)?d:NULL)
#endif

#define INS_BUFSZ  4096

/* ---- Callback types ---- */
typedef void (*InsProgressFn)(const char *name, long done, long total,
                              void *user);
typedef void (*InsMessageFn)(const char *msg, void *user);
typedef int  (*InsFilterFn)(const char *name, void *user);

/* ---- Opaque context ---- */
typedef struct InsCtx InsCtx;

/* ---- Lifecycle ---- */
InsCtx *insCtxNew(void);
void    insCtxFree(InsCtx *ctx);

/* ---- Callback registration ---- */
void insSetProgressCb(InsCtx *ctx, InsProgressFn fn, void *user);
void insSetMessageCb(InsCtx *ctx, InsMessageFn fn, void *user);

/* ---- Packing ---- */
int  insOpen(InsCtx *ctx, const char *path);
int  insAddFile(InsCtx *ctx, const char *path);
int  insAddFileAs(InsCtx *ctx, const char *realPath, const char *archiveName);
int  insAddDir(InsCtx *ctx, const char *name);
int  insAddBanner(InsCtx *ctx, const char *path);
int  insAddCommand(InsCtx *ctx, const char *description, const char *command);
int  insClose(InsCtx *ctx);

/* ---- Packing enhancements ---- */
int insAddWildcard(InsCtx *ctx, const char *pattern);
int insAddWildcardAs(InsCtx *ctx, const char *pattern, const char *archivePrefix);
int insAddTree(InsCtx *ctx, const char *dir,
               InsFilterFn filter, void *filterUser);

/* ---- Extraction ---- */
int insExtractAll(InsCtx *ctx, const char *archive);
int insExtractOne(InsCtx *ctx, const char *archive, const char *name);

/* ---- Utility ---- */
int  insCopyFile(InsCtx *ctx, const char *src, const char *dst);
int  insMkdir(const char *path);

/* ---- Info ---- */
int  insEntryCount(InsCtx *ctx);
long insFileSize(const char *path);

/* ---- Internal context structure (visible for .c files) ---- */
struct InsCtx {
    FILE      *out;
    int        count;
    int        opened;
    char       buf[INS_BUFSZ];

    InsProgressFn onProgress;
    InsMessageFn  onMessage;
    void         *progressUser;
    void         *messageUser;
};

/* ---- Internal helpers ---- */
void insFireProgress(InsCtx *ctx, const char *name, long done, long total);
void insFireMessage(InsCtx *ctx, const char *fmt, ...);
int  insMatch(const char *pattern, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* INSTALLER_H */
