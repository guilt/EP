#include "INSTALL.H"
#include <stdarg.h>

#if defined(__MSDOS__) || defined(__BORLANDC__)
# include <io.h>
# include <dos.h>
#elif defined(_WIN32)
# include <windows.h>
#else
# include <dirent.h>
#endif

/* ---- snprintf fallback for DOS ---- */
#if defined(__MSDOS__) || defined(__BORLANDC__)
int insSnprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    int ret;
    va_start(ap, fmt);
    ret = vsprintf(buf, fmt, ap);
    va_end(ap);
    (void)n;
    return ret;
}
int insVsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
    int ret = vsprintf(buf, fmt, ap);
    (void)n;
    return ret;
}
#endif

/* ===== Context management ===== */

InsCtx *insCtxNew(void)
{
    InsCtx *ctx = (InsCtx *)calloc(1, sizeof(InsCtx));
    if (!ctx) return NULL;
    ctx->out          = NULL;
    ctx->count        = 0;
    ctx->opened       = 0;
    ctx->onProgress   = NULL;
    ctx->onMessage    = NULL;
    ctx->progressUser = NULL;
    ctx->messageUser  = NULL;
    return ctx;
}

void insCtxFree(InsCtx *ctx)
{
    if (!ctx) return;
    if (ctx->opened && ctx->out)
        insClose(ctx);
    free(ctx);
}

void insSetProgressCb(InsCtx *ctx, InsProgressFn fn, void *user)
{
    if (!ctx) return;
    ctx->onProgress   = fn;
    ctx->progressUser = user;
}

void insSetMessageCb(InsCtx *ctx, InsMessageFn fn, void *user)
{
    if (!ctx) return;
    ctx->onMessage   = fn;
    ctx->messageUser = user;
}

void insFireProgress(InsCtx *ctx, const char *name, long done, long total)
{
    if (ctx && ctx->onProgress)
        ctx->onProgress(name, done, total, ctx->progressUser);
}

void insFireMessage(InsCtx *ctx, const char *fmt, ...)
{
    char msg[512];
    va_list ap;
    if (!ctx || !ctx->onMessage) return;
    va_start(ap, fmt);
    insVsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    ctx->onMessage(msg, ctx->messageUser);
}

int insEntryCount(InsCtx *ctx)
{
    return ctx ? ctx->count : 0;
}

long insFileSize(const char *path)
{
    FILE *f;
    long sz;
    f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fclose(f);
    return sz;
}

int insMkdir(const char *path)
{
    return INS_MKDIR(path);
}

/* ===== Wildcard matching ===== */

int insMatch(const char *pat, const char *name)
{
    while (*pat)
    {
        if (*pat == '*')
        {
            pat++;
            if (*pat == '\0') return 1;
            while (*name)
            {
                if (insMatch(pat, name)) return 1;
                name++;
            }
            return 0;
        }
        if (*pat == '?')
        {
            if (*name == '\0') return 0;
            pat++; name++;
            continue;
        }
        if (*pat != *name) return 0;
        pat++; name++;
    }
    return *name == '\0';
}

static const char *splitName(const char *pattern, char *dirBuf, int dirLen)
{
    const char *slash, *bs, *sep;
    int len;
    slash = strrchr(pattern, '/');
    bs    = strrchr(pattern, '\\');
    sep   = (slash > bs) ? slash : bs;
    if (sep)
    {
        len = (int)(sep - pattern);
        if (len >= dirLen) len = dirLen - 1;
        memcpy(dirBuf, pattern, len);
        dirBuf[len] = '\0';
        return sep + 1;
    }
    dirBuf[0] = '\0';
    return pattern;
}

/* ===== Wildcard add ===== */

int insAddWildcardAs(InsCtx *ctx, const char *pattern, const char *archivePrefix)
{
    char dir[512];
    const char *filePat;
    int rc, prefixLen;
    if (!ctx || !pattern) return INS_EINVAL;
    filePat = splitName(pattern, dir, sizeof(dir));
    prefixLen = archivePrefix ? (int)strlen(archivePrefix) : 0;

#if defined(__MSDOS__) || defined(__BORLANDC__)
    {
        struct find_t fd;
        char query[512];
        int dosrc;
        if (dir[0]) insSnprintf(query, sizeof(query), "%s\\*", dir);
        else        insSnprintf(query, sizeof(query), "*");
        dosrc = _dos_findfirst(query, _A_NORMAL, &fd);
        if (dosrc != 0) return INS_ENOTFOUND;
        do {
            char full[512], arcName[512];
            if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
            if (fd.attrib & _A_SUBDIR) continue;
            if (!insMatch(filePat, fd.name)) continue;
            if (dir[0]) insSnprintf(full, sizeof(full), "%s\\%s", dir, fd.name);
            else        insSnprintf(full, sizeof(full), "%s", fd.name);
            if (prefixLen > 0)
                insSnprintf(arcName, sizeof(arcName), "%s%s", archivePrefix, fd.name);
            else if (dir[0])
                insSnprintf(arcName, sizeof(arcName), "%s\\%s", dir, fd.name);
            else
                insSnprintf(arcName, sizeof(arcName), "%s", fd.name);
            rc = insAddFileAs(ctx, full, arcName);
            if (rc != INS_OK) return rc;
            dosrc = _dos_findnext(&fd);
        } while (dosrc == 0);
    }
#elif defined(_WIN32)
    {
        struct _finddata_t fd;
        long handle;
        char query[512];
        if (dir[0]) insSnprintf(query, sizeof(query), "%s\\*", dir);
        else        insSnprintf(query, sizeof(query), "*");
        handle = _findfirst(query, &fd);
        if (handle == -1L) return INS_ENOTFOUND;
        do {
            char full[512], arcName[512];
            if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
            if (fd.attrib & _A_SUBDIR) continue;
            if (!insMatch(filePat, fd.name)) continue;
            if (dir[0]) insSnprintf(full, sizeof(full), "%s\\%s", dir, fd.name);
            else        insSnprintf(full, sizeof(full), "%s", fd.name);
            if (prefixLen > 0)
                insSnprintf(arcName, sizeof(arcName), "%s%s", archivePrefix, fd.name);
            else if (dir[0])
                insSnprintf(arcName, sizeof(arcName), "%s\\%s", dir, fd.name);
            else
                insSnprintf(arcName, sizeof(arcName), "%s", fd.name);
            rc = insAddFileAs(ctx, full, arcName);
            if (rc != INS_OK) { _findclose(handle); return rc; }
        } while (_findnext(handle, &fd) == 0);
        _findclose(handle);
    }
#else
    {
        const char *searchDir = dir[0] ? dir : ".";
        DIR *d = opendir(searchDir);
        if (!d) return INS_ENOTFOUND;
        struct dirent *de;
        while ((de = readdir(d)) != NULL)
        {
            struct stat st;
            char full[512], arcName[512];
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
            if (!insMatch(filePat, de->d_name)) continue;
            if (dir[0]) insSnprintf(full, sizeof(full), "%s/%s", dir, de->d_name);
            else        insSnprintf(full, sizeof(full), "%s", de->d_name);
            if (stat(full, &st) != 0 || S_ISDIR(st.st_mode)) continue;
            if (prefixLen > 0)
                insSnprintf(arcName, sizeof(arcName), "%s%s", archivePrefix, de->d_name);
            else if (dir[0])
                insSnprintf(arcName, sizeof(arcName), "%s/%s", dir, de->d_name);
            else
                insSnprintf(arcName, sizeof(arcName), "%s", de->d_name);
            rc = insAddFileAs(ctx, full, arcName);
            if (rc != INS_OK) { closedir(d); return rc; }
        }
        closedir(d);
    }
#endif
    insFireMessage(ctx, "Added files matching: %s", pattern);
    return INS_OK;
}

int insAddWildcard(InsCtx *ctx, const char *pattern)
{
    return insAddWildcardAs(ctx, pattern, NULL);
}

/* ===== Tree add ===== */

int insAddTree(InsCtx *ctx, const char *dir, InsFilterFn filter, void *filterUser)
{
    int rc;
    if (!ctx || !dir) return INS_EINVAL;

#if defined(__MSDOS__) || defined(__BORLANDC__)
    {
        struct find_t fd;
        char query[512];
        int dosrc;
        insSnprintf(query, sizeof(query), "%s\\*", dir);
        dosrc = _dos_findfirst(query, _A_NORMAL | _A_SUBDIR, &fd);
        if (dosrc != 0) return INS_OK;
        do {
            char full[512], arcName[512];
            if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
            insSnprintf(full, sizeof(full), "%s\\%s", dir, fd.name);
            if (fd.attrib & _A_SUBDIR)
            {
                insSnprintf(arcName, sizeof(arcName), "%s\\%s", dir, fd.name);
                insAddDir(ctx, arcName);
                insAddTree(ctx, full, filter, filterUser);
            }
            else
            {
                if (!filter || filter(full, filterUser))
                {
                    insSnprintf(arcName, sizeof(arcName), "%s\\%s", dir, fd.name);
                    rc = insAddFileAs(ctx, full, arcName);
                    if (rc != INS_OK) return rc;
                }
            }
            dosrc = _dos_findnext(&fd);
        } while (dosrc == 0);
    }
#elif defined(_WIN32)
    {
        struct _finddata_t fd;
        long handle;
        char query[512];
        insSnprintf(query, sizeof(query), "%s\\*", dir);
        handle = _findfirst(query, &fd);
        if (handle == -1L) return INS_OK;
        do {
            char full[512], arcName[512];
            if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
            insSnprintf(full, sizeof(full), "%s\\%s", dir, fd.name);
            if (fd.attrib & _A_SUBDIR)
            {
                insSnprintf(arcName, sizeof(arcName), "%s\\%s", dir, fd.name);
                insAddDir(ctx, arcName);
                insAddTree(ctx, full, filter, filterUser);
            }
            else
            {
                if (!filter || filter(full, filterUser))
                {
                    insSnprintf(arcName, sizeof(arcName), "%s\\%s", dir, fd.name);
                    rc = insAddFileAs(ctx, full, arcName);
                    if (rc != INS_OK) { _findclose(handle); return rc; }
                }
            }
        } while (_findnext(handle, &fd) == 0);
        _findclose(handle);
    }
#else
    {
        DIR *d = opendir(dir);
        if (!d) return INS_OK;
        struct dirent *de;
        while ((de = readdir(d)) != NULL)
        {
            struct stat st;
            char full[512], arcName[512];
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
            insSnprintf(full, sizeof(full), "%s/%s", dir, de->d_name);
            if (stat(full, &st) != 0) continue;
            if (S_ISDIR(st.st_mode))
            {
                insSnprintf(arcName, sizeof(arcName), "%s/%s", dir, de->d_name);
                insAddDir(ctx, arcName);
                insAddTree(ctx, full, filter, filterUser);
            }
            else if (S_ISREG(st.st_mode))
            {
                if (!filter || filter(full, filterUser))
                {
                    insSnprintf(arcName, sizeof(arcName), "%s/%s", dir, de->d_name);
                    rc = insAddFileAs(ctx, full, arcName);
                    if (rc != INS_OK) { closedir(d); return rc; }
                }
            }
        }
        closedir(d);
    }
#endif
    insFireMessage(ctx, "Added tree: %s", dir);
    return INS_OK;
}

/* ===== Copy file ===== */

int insCopyFile(InsCtx *ctx, const char *src, const char *dst)
{
    FILE *in, *out;
    char localBuf[INS_BUFSZ], *buf;
    long total, done;
    int chunk;
    size_t nr;
    if (!src || !dst) return INS_EINVAL;
    in = fopen(src, "rb");
    if (!in) return INS_ECANTOPENR;
    out = fopen(dst, "wb");
    if (!out) { fclose(in); return INS_ECANTOPENW; }
    buf = ctx ? ctx->buf : localBuf;
    fseek(in, 0, SEEK_END);
    total = ftell(in);
    fseek(in, 0, SEEK_SET);
    done = 0;
    while (done < total)
    {
        chunk = (total - done > INS_BUFSZ) ? INS_BUFSZ : (int)(total - done);
        nr = fread(buf, 1, chunk, in);
        if (nr == 0) break;
        fwrite(buf, 1, nr, out);
        done += (long)nr;
        if (ctx) insFireProgress(ctx, src, done, total);
    }
    fclose(out); fclose(in);
    if (ctx) insFireMessage(ctx, "Copied: %s -> %s (%ld bytes)", src, dst, total);
    return INS_OK;
}

/* ===== Pack functions ===== */

static int writeEntry(InsCtx *ctx, FILE *in, const char *name,
                      InsU32 contentLen, InsU16 dflag)
{
    InsU16 nameLen;
    InsU32 remaining;
    int chunk;
    size_t nr;
    nameLen = (InsU16)strlen(name);
    fwrite(&nameLen, sizeof(InsU16), 1, ctx->out);
    fwrite(name, 1, nameLen, ctx->out);
    fwrite(&dflag, sizeof(InsU16), 1, ctx->out);
    if (dflag == 1) { ctx->count++; return INS_OK; }
    fwrite(&contentLen, sizeof(InsU32), 1, ctx->out);
    remaining = contentLen;
    while (remaining > 0)
    {
        chunk = (remaining > INS_BUFSZ) ? INS_BUFSZ : (int)remaining;
        nr = fread(ctx->buf, 1, chunk, in);
        if (nr == 0) break;
        fwrite(ctx->buf, 1, nr, ctx->out);
        remaining -= (InsU32)nr;
        insFireProgress(ctx, name, (long)(contentLen - remaining), (long)contentLen);
    }
    ctx->count++;
    return INS_OK;
}

int insOpen(InsCtx *ctx, const char *path)
{
    InsU16 zero = 0;
    if (!ctx || !path) return INS_EINVAL;
    ctx->out = fopen(path, "wb");
    if (!ctx->out) return INS_ECANTOPENW;
    fwrite("PF", 1, 2, ctx->out);
    fwrite(&zero, sizeof(InsU16), 1, ctx->out);
    ctx->count = 0; ctx->opened = 1;
    insFireMessage(ctx, "Opened archive: %s", path);
    return INS_OK;
}

int insAddFileAs(InsCtx *ctx, const char *realPath, const char *archiveName)
{
    FILE *in;
    long flen;
    int rc;
    if (!ctx || !ctx->opened || !ctx->out) return INS_EINVAL;
    if (!realPath || *realPath == '\0') return INS_EINVAL;
    if (!archiveName || *archiveName == '\0') return INS_EINVAL;
    in = fopen(realPath, "rb");
    if (!in) return INS_ENOTFOUND;
    fseek(in, 0, SEEK_END); flen = ftell(in); fseek(in, 0, SEEK_SET);
    rc = writeEntry(ctx, in, archiveName, (InsU32)flen, 0);
    fclose(in);
    if (rc == INS_OK)
        insFireMessage(ctx, "Packed: %s -> %s (%ld bytes)", realPath, archiveName, flen);
    return rc;
}

int insAddFile(InsCtx *ctx, const char *path)
{
    return insAddFileAs(ctx, path, path);
}

int insAddDir(InsCtx *ctx, const char *name)
{
    int rc;
    if (!ctx || !ctx->opened || !ctx->out) return INS_EINVAL;
    if (!name || *name == '\0') return INS_EINVAL;
    rc = writeEntry(ctx, NULL, name, 0, 1);
    if (rc == INS_OK) insFireMessage(ctx, "Added directory: %s", name);
    return rc;
}

int insAddBanner(InsCtx *ctx, const char *path)
{
    FILE *in; long flen; int rc;
    if (!ctx || !ctx->opened || !ctx->out) return INS_EINVAL;
    if (!path || *path == '\0') return INS_EINVAL;
    in = fopen(path, "rb");
    if (!in) return INS_ENOTFOUND;
    fseek(in, 0, SEEK_END); flen = ftell(in); fseek(in, 0, SEEK_SET);
    rc = writeEntry(ctx, in, path, (InsU32)flen, 2);
    fclose(in);
    if (rc == INS_OK) insFireMessage(ctx, "Added banner: %s (%ld bytes)", path, flen);
    return rc;
}

int insAddCommand(InsCtx *ctx, const char *description, const char *command)
{
    InsU16 dflag = 3;
    InsU16 nameLen;
    InsU32 cmdLen;
    if (!ctx || !ctx->opened || !ctx->out) return INS_EINVAL;
    if (!description || !command) return INS_EINVAL;
    nameLen = (InsU16)strlen(description);
    cmdLen  = (InsU32)strlen(command);
    fwrite(&nameLen, sizeof(InsU16), 1, ctx->out);
    fwrite(description, 1, nameLen, ctx->out);
    fwrite(&dflag, sizeof(InsU16), 1, ctx->out);
    fwrite(&cmdLen, sizeof(InsU32), 1, ctx->out);
    fwrite(command, 1, cmdLen, ctx->out);
    ctx->count++;
    insFireMessage(ctx, "Added command: %s -> %s", description, command);
    return INS_OK;
}

int insClose(InsCtx *ctx)
{
    InsU16 count16;
    if (!ctx || !ctx->opened || !ctx->out) return INS_EINVAL;
    count16 = (InsU16)ctx->count;
    fseek(ctx->out, 0, SEEK_SET);
    fwrite("PF", 1, 2, ctx->out);
    fwrite(&count16, sizeof(InsU16), 1, ctx->out);
    fclose(ctx->out);
    ctx->out = NULL; ctx->opened = 0;
    insFireMessage(ctx, "Finalized archive: %d entries", ctx->count);
    return INS_OK;
}

/* ===== Extract functions ===== */

static int readAll(FILE *f, void *buf, size_t n)
{
    return fread(buf, 1, n, f) != n;
}

static void mkdirParents(const char *path)
{
    char tmp[512];
    int i;
    insSnprintf(tmp, sizeof(tmp), "%s", path);
    for (i = 0; tmp[i]; i++)
    {
        if (tmp[i] == '/' || tmp[i] == '\\')
        {
            char saved = tmp[i];
            tmp[i] = '\0';
            mkDir(tmp);
            tmp[i] = saved;
        }
    }
}

static int extractCore(InsCtx *ctx, const char *archive,
                       const char *target, int extractAll)
{
    FILE *ar;
    char magic[2];
    InsU16 total16;
    int total, found, i;
    if (!ctx || !archive) return INS_EINVAL;
    ar = fopen(archive, "rb");
    if (!ar) return INS_ECANTOPENR;
    if (readAll(ar, magic, 2) || magic[0] != 'P' || magic[1] != 'F')
    { fclose(ar); return INS_EBADFORMAT; }
    if (readAll(ar, &total16, sizeof(InsU16)))
    { fclose(ar); return INS_EBADFORMAT; }
    total = total16;
    insFireMessage(ctx, "Extracting from: %s (%d entries)", archive, total);
    found = 0;
    for (i = 0; i < total; i++)
    {
        InsU16 nameLen16;
        int nameLen;
        char *name;
        InsU16 dflag;
        if (readAll(ar, &nameLen16, sizeof(InsU16))) break;
        nameLen = nameLen16;
        name = (char *)malloc(nameLen + 1);
        if (!name) { fclose(ar); return INS_ENOMEM; }
        if (readAll(ar, name, nameLen)) { free(name); break; }
        name[nameLen] = '\0';
        if (readAll(ar, &dflag, sizeof(InsU16))) { free(name); break; }

        if (dflag == 0)
        {
            InsU32 contentLen32; long contentLen; int match;
            if (readAll(ar, &contentLen32, sizeof(InsU32))) { free(name); break; }
            contentLen = (long)contentLen32;
            match = extractAll || (target && strcmp(name, target) == 0);
            if (match)
            {
                FILE *out; long remaining; int chunk; size_t nr;
                mkdirParents(name);
                out = fopen(name, "wb");
                if (!out) { fseek(ar, contentLen, SEEK_CUR); free(name); fclose(ar); return INS_ECANTOPENW; }
                remaining = contentLen;
                while (remaining > 0)
                {
                    chunk = (remaining > INS_BUFSZ) ? INS_BUFSZ : (int)remaining;
                    nr = fread(ctx->buf, 1, chunk, ar);
                    if (nr == 0) break;
                    fwrite(ctx->buf, 1, nr, out);
                    remaining -= (long)nr;
                    insFireProgress(ctx, name, contentLen - remaining, contentLen);
                }
                fclose(out); found = 1;
                insFireMessage(ctx, "Extracted: %s (%ld bytes)", name, contentLen);
                if (!extractAll) { free(name); break; }
            }
            else { fseek(ar, contentLen, SEEK_CUR); }
        }
        else if (dflag == 1)
        {
            int match = extractAll || (target && strcmp(name, target) == 0);
            if (match) { mkDir(name); found = 1; insFireMessage(ctx, "Created directory: %s", name); if (!extractAll) { free(name); break; } }
        }
        else if (dflag == 2)
        {
            InsU32 contentLen32; long contentLen;
            if (readAll(ar, &contentLen32, sizeof(InsU32))) { free(name); break; }
            contentLen = (long)contentLen32;
            if (contentLen > 0 && contentLen < 4096)
            {
                char *bannerText = (char *)malloc(contentLen + 1);
                if (bannerText)
                {
                    if (readAll(ar, bannerText, contentLen)) { free(bannerText); free(name); break; }
                    bannerText[contentLen] = '\0';
                    insFireMessage(ctx, "\n%s\n", bannerText);
                    free(bannerText);
                }
                else { fseek(ar, contentLen, SEEK_CUR); }
            }
            else { fseek(ar, contentLen, SEEK_CUR); }
            insFireMessage(ctx, "Displayed banner: %s (%ld bytes)", name, contentLen);
        }
        else if (dflag == 3)
        {
            InsU32 cmdLen32;
            if (readAll(ar, &cmdLen32, sizeof(InsU32))) { free(name); break; }
            if (cmdLen32 > 0 && cmdLen32 < 4096)
            {
                char *cmdText = (char *)malloc(cmdLen32 + 1);
                if (cmdText)
                {
                    if (readAll(ar, cmdText, cmdLen32)) { free(cmdText); free(name); break; }
                    cmdText[cmdLen32] = '\0';
                    insFireMessage(ctx, "Command: %s -> %s", name, cmdText);
                    free(cmdText);
                }
                else { fseek(ar, cmdLen32, SEEK_CUR); }
            }
            else { fseek(ar, cmdLen32, SEEK_CUR); }
        }
        free(name);
    }
    fclose(ar);
    insFireMessage(ctx, "Extraction complete");
    return (extractAll || found) ? INS_OK : INS_ENOTFOUND;
}

int insExtractAll(InsCtx *ctx, const char *archive)
{
    return extractCore(ctx, archive, NULL, 1);
}

int insExtractOne(InsCtx *ctx, const char *archive, const char *name)
{
    if (!name) return INS_EINVAL;
    return extractCore(ctx, archive, name, 0);
}
