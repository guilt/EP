/* CLI packer — reads .EPI manifest, creates .PAK or self-extracting .EXE */
#include "INSTALL.H"
#include "VER.H"
#include <stdio.h>
#include <string.h>

static void onProgress(const char *name, long done, long total, void *u)
{
    (void)u;
    printf("\r  %s: %ld/%ld KB  ", name, done / 1000, total / 1000);
}

static void onMessage(const char *msg, void *u)
{
    (void)u;
    printf("\n[INFO] %s\n", msg);
}

static void printBanner(void)
{
    printf("\n  Installer Packer v%s\n", VProductVersion);
    printf("  %s\n", VLegalCopyright);
    printf("  -----------------------------------------\n\n");
}

static int appendFile(FILE *out, const char *src)
{
    FILE *in = fopen(src, "rb");
    char buf[4096];
    size_t nr;
    if (!in) return -1;
    while ((nr = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, nr, out);
    fclose(in);
    return 0;
}

static const char *archiveName(const char *path)
{
    while (path[0] == '.' && path[1] == '.' && (path[2] == '/' || path[2] == '\\'))
        path += 3;
    return path;
}

static void archivePrefix(const char *pattern, char *out, int outLen)
{
    const char *slash, *bs, *sep, *norm;
    int len;
    slash = strrchr(pattern, '/');
    bs    = strrchr(pattern, '\\');
    sep   = (slash > bs) ? slash : bs;
    if (!sep) { out[0] = '\0'; return; }
    len = (int)(sep - pattern + 1);
    if (len >= outLen) len = outLen - 1;
    memcpy(out, pattern, len);
    out[len] = '\0';
    norm = archiveName(out);
    insSnprintf(out, outLen, "%s", norm);
}

int main(int argc, char *argv[])
{
    InsCtx *ctx;
    FILE *mf, *outFile;
    char line[512], outpath[512], manifest[512], extractor[512];
    int rc, linenum, argi, sfxMode, totalEntries;
    long palOffset;
    unsigned long offset32;

    printBanner();

    if (argc < 2)
    {
        printf("Usage: %s [-C <dir>] [-S] [-X <exe>] manifest.epi [output]\n"
               "  -C <dir>   Change to <dir> before processing\n"
               "  -S         Create self-extracting .EXE\n"
               "  -X <exe>   Extractor to prepend (default: ..\\BIN\\EXTRACT.EXE)\n"
               "  output     .PAK or .EXE with -S\n\n", argv[0]);
        return 1;
    }

    sfxMode = 0;
    insSnprintf(extractor, sizeof(extractor), "..\\BIN\\EXTRACT.EXE");
    argi = 1;

    while (argi < argc && argv[argi][0] == '-')
    {
        if (strcmp(argv[argi], "-C") == 0 && argi + 1 < argc)
        {
            if (chDir(argv[argi + 1]) != 0)
            { printf("Failed to change dir: %s\n", argv[argi + 1]); return 1; }
            printf("  Changed to: %s\n\n", argv[argi + 1]);
            argi += 2;
        }
        else if (strcmp(argv[argi], "-S") == 0) { sfxMode = 1; argi++; }
        else if (strcmp(argv[argi], "-X") == 0 && argi + 1 < argc)
        { insSnprintf(extractor, sizeof(extractor), "%s", argv[argi + 1]); argi += 2; }
        else { printf("Unknown: %s\n", argv[argi]); return 1; }
    }

    if (argi >= argc) { printf("Missing manifest\n"); return 1; }
    insSnprintf(manifest, sizeof(manifest), "%s", argv[argi]);
    if (argi + 1 < argc) insSnprintf(outpath, sizeof(outpath), "%s", argv[argi + 1]);
    else insSnprintf(outpath, sizeof(outpath), sfxMode ? "OUTPUT.EXE" : "OUTPUT.PAK");

    printf("  Manifest: %s\n  Output:   %s\n", manifest, outpath);
    if (sfxMode) printf("  SFX: %s\n", extractor);
    printf("\n");

    mf = fopen(manifest, "r");
    if (!mf) { printf("Cannot open %s\n", manifest); return 1; }

    ctx = insCtxNew();
    insSetProgressCb(ctx, onProgress, NULL);
    insSetMessageCb(ctx, onMessage, NULL);

    rc = insOpen(ctx, "$$TMPPAK$$");
    if (rc != INS_OK) { printf("Failed to create temp (%d)\n", rc); fclose(mf); insCtxFree(ctx); return 1; }

    linenum = 0;
    while (fgets(line, sizeof(line), mf))
    {
        char *p;
        linenum++;
        p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;

        if (p[0] == 'F' && (p[1] == ' ' || p[1] == '\t'))
        {
            char *path = p + 2;
            while (*path == ' ' || *path == '\t') path++;
            path[strcspn(path, "\r\n")] = '\0';
            if (!*path) continue;
            if (strpbrk(path, "*?"))
            {
                char prefix[512];
                archivePrefix(path, prefix, sizeof(prefix));
                rc = insAddWildcardAs(ctx, path, prefix);
            }
            else
                rc = insAddFileAs(ctx, path, archiveName(path));
            if (rc != INS_OK)
                printf("  [WARN] line %d: '%s' (%d)\n", linenum, path, rc);
        }
        else if (p[0] == 'D' && (p[1] == ' ' || p[1] == '\t'))
        {
            char *path = p + 2;
            while (*path == ' ' || *path == '\t') path++;
            path[strcspn(path, "\r\n")] = '\0';
            if (*path) { rc = insAddDir(ctx, archiveName(path)); if (rc != INS_OK) printf("  [WARN] line %d: dir '%s' (%d)\n", linenum, path, rc); }
        }
        else if (p[0] == 'C' && (p[1] == ' ' || p[1] == '\t'))
        {
            char *desc = p + 2, *cmd;
            while (*desc == ' ' || *desc == '\t') desc++;
            desc[strcspn(desc, "\r\n")] = '\0';
            cmd = strchr(desc, '|');
            if (cmd) { *cmd = '\0'; cmd++; while (*cmd == ' ' || *cmd == '\t') cmd++; if (*desc && *cmd) { rc = insAddCommand(ctx, desc, cmd); if (rc != INS_OK) printf("  [WARN] line %d: cmd (%d)\n", linenum, rc); } }
            else printf("  [WARN] line %d: C desc|cmd\n", linenum);
        }
        else if (p[0] == 'B' && (p[1] == ' ' || p[1] == '\t'))
        {
            char *path = p + 2;
            while (*path == ' ' || *path == '\t') path++;
            path[strcspn(path, "\r\n")] = '\0';
            if (*path) { rc = insAddBanner(ctx, path); if (rc != INS_OK) printf("  [WARN] line %d: banner '%s' (%d)\n", linenum, path, rc); }
        }
        else printf("  [WARN] line %d: '%s'\n", linenum, line);
    }

    fclose(mf);
    rc = insClose(ctx);
    if (rc != INS_OK) { printf("Failed to finalize (%d)\n", rc); insCtxFree(ctx); return 1; }

    totalEntries = insEntryCount(ctx);
    printf("\n  Archive: %d entries\n", totalEntries);
    insCtxFree(ctx);

    outFile = fopen(outpath, "wb");
    if (!outFile) { printf("Cannot create: %s\n", outpath); remove("$$TMPPAK$$"); return 1; }

    if (sfxMode)
    {
        if (appendFile(outFile, extractor) != 0)
        { printf("Cannot open extractor: %s\n", extractor); fclose(outFile); remove("$$TMPPAK$$"); remove(outpath); return 1; }
        palOffset = ftell(outFile);
        printf("  Extractor stub: %ld bytes\n", palOffset);
    }
    else palOffset = 0;

    if (appendFile(outFile, "$$TMPPAK$$") != 0)
    { printf("Failed to read temp\n"); fclose(outFile); remove("$$TMPPAK$$"); remove(outpath); return 1; }

    if (sfxMode) { offset32 = (unsigned long)palOffset; fwrite(&offset32, sizeof(unsigned long), 1, outFile); }

    fclose(outFile);
    remove("$$TMPPAK$$");
    printf("\n  Created: %s (%d entries)\n", outpath, totalEntries);
    return 0;
}
