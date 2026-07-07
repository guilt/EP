/* CLI extractor — extracts .PAK archives or runs as SFX */
#include "INSTALL.H"
#include "VER.H"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void onProgress(const char *name, long done, long total, void *u)
{
    (void)u;
    printf("\r  %s: %ld/%ld bytes  ", name, done, total);
}

static void onMessage(const char *msg, void *u)
{
    (void)u;
    printf("\n[INFO] %s\n", msg);
}

static void printBanner(void)
{
    printf("\n  Installer Extractor v%s\n", VProductVersion);
    printf("  %s\n", VLegalCopyright);
    printf("  -----------------------------------------\n\n");
}

static int ask(const char *prompt)
{
    char reply[16];
    printf("\n  %s (Y/N): ", prompt);
    if (!fgets(reply, sizeof(reply), stdin)) return 0;
    return (reply[0] == 'Y' || reply[0] == 'y');
}

/* Scan archive for command entries (dflag=3) */
static int collectCommands(const char *archive, char descs[][128], char cmds[][128], int max)
{
    FILE *ar;
    unsigned short total16, nameLen16, dflag;
    int total, count, i;
    char magic[2];
    ar = fopen(archive, "rb");
    if (!ar) return 0;
    if (fread(magic, 1, 2, ar) != 2 || magic[0] != 'P' || magic[1] != 'F') { fclose(ar); return 0; }
    if (fread(&total16, 2, 1, ar) != 1) { fclose(ar); return 0; }
    total = total16;
    count = 0;
    for (i = 0; i < total && count < max; i++)
    {
        unsigned long skip;
        char *name;
        if (fread(&nameLen16, 2, 1, ar) != 1) break;
        name = (char *)malloc(nameLen16 + 1);
        if (!name) break;
        if (fread(name, 1, nameLen16, ar) != nameLen16) { free(name); break; }
        name[nameLen16] = '\0';
        if (fread(&dflag, 2, 1, ar) != 1) { free(name); break; }
        if (dflag == 3)
        {
            unsigned long cmdLen32;
            if (fread(&cmdLen32, 4, 1, ar) != 1) { free(name); break; }
            if (cmdLen32 < 128) { fread(cmds[count], 1, cmdLen32, ar); cmds[count][cmdLen32] = '\0'; }
            else { fseek(ar, cmdLen32, SEEK_CUR); cmds[count][0] = '\0'; }
            insSnprintf(descs[count], 128, "%s", name);
            count++;
        }
        else if (dflag == 0 || dflag == 2)
        {
            if (fread(&skip, 4, 1, ar) != 1) { free(name); break; }
            fseek(ar, skip, SEEK_CUR);
        }
        free(name);
    }
    fclose(ar);
    return count;
}

int main(int argc, char *argv[])
{
    InsCtx *ctx;
    FILE *ar;
    unsigned long offset;
    const char *archive;
    char absArchive[512], sfxTmp[512], outDir[256], cwd[512];
    int rc, argi, useSfx, cmdCount;
    char cmdDescs[16][128];
    char cmdLines[16][128];

    printBanner();
    outDir[0] = '\0';
    argi = 1;

    if (argc > 2 && strcmp(argv[1], "-D") == 0)
    { insSnprintf(outDir, sizeof(outDir), "%s", argv[2]); argi = 3; }

    if (getCwd(cwd, sizeof(cwd)) == NULL) insSnprintf(cwd, sizeof(cwd), ".");

    if (argi >= argc)
    {
        useSfx = 1;
        archive = argv[0];
        printf("  SFX mode: %s\n", archive);
        ar = fopen(archive, "rb");
        if (!ar) { printf("Cannot open self\n"); return 1; }
        fseek(ar, -4, SEEK_END);
        if (fread(&offset, sizeof(unsigned long), 1, ar) != 1) { fclose(ar); printf("Invalid SFX\n"); return 1; }
        fclose(ar);
        printf("  PAL data offset: %lu bytes\n", offset);

        { const char *_fp = fullPath(sfxTmp, "$$TMPEXT$$", sizeof(sfxTmp)); (void)_fp; }
        {
            FILE *realAr, *markerFile;
            char buf[4096];
            size_t nr;
            long remaining, palSize;
            int chunk;
            realAr = fopen(archive, "rb");
            if (!realAr) { printf("Cannot re-open\n"); return 1; }
            fseek(realAr, 0, SEEK_END); palSize = ftell(realAr) - offset - 4;
            fclose(realAr);
            realAr = fopen(archive, "rb");
            markerFile = fopen(sfxTmp, "wb");
            if (!markerFile) { fclose(realAr); printf("Cannot create temp\n"); return 1; }
            fseek(realAr, (long)offset, SEEK_SET);
            remaining = palSize;
            while (remaining > 0)
            {
                chunk = (remaining > 4096) ? 4096 : (int)remaining;
                nr = fread(buf, 1, chunk, realAr);
                if (nr == 0) break;
                fwrite(buf, 1, nr, markerFile);
                remaining -= (long)nr;
            }
            fclose(markerFile); fclose(realAr);
            archive = sfxTmp;
        }
    }
    else
    {
        useSfx = 0;
        if (fullPath(absArchive, argv[argi], sizeof(absArchive)))
            archive = absArchive;
        else
            archive = argv[argi];
        printf("  Archive: %s\n", archive);
    }

    {
        const char *nameSrc, *base, *bs2, *p, *dot;
        int len;
        nameSrc = (argi < argc) ? argv[argi] : argv[0];
        if (outDir[0] == '\0')
        {
            base = strrchr(nameSrc, '/');
            bs2 = strrchr(nameSrc, '\\');
            p = (base > bs2) ? base : bs2;
            if (!p) p = nameSrc; else p++;
            dot = strrchr(p, '.');
            len = dot ? (int)(dot - p) : (int)strlen(p);
            if (len > 255) len = 255;
            memcpy(outDir, p, len);
            outDir[len] = '\0';
            if (len == 0) insSnprintf(outDir, sizeof(outDir), "extracted");
        }
    }

    printf("  Output dir: %s\n\n", outDir);
    mkDir(outDir);
    if (chDir(outDir) != 0) { printf("Failed to enter %s\n", outDir); if (useSfx) remove(sfxTmp); return 1; }

    ctx = insCtxNew();
    insSetProgressCb(ctx, onProgress, NULL);
    insSetMessageCb(ctx, onMessage, NULL);
    rc = insExtractAll(ctx, archive);
    if (rc != INS_OK) { printf("\n  Extraction failed (%d)\n", rc); insCtxFree(ctx); chDir(cwd); if (useSfx) remove(sfxTmp); return 1; }
    insCtxFree(ctx);
    printf("\n  Extraction complete.\n");

    chDir(cwd);
    if (useSfx) archive = sfxTmp;

    cmdCount = collectCommands(archive, cmdDescs, cmdLines, 16);
    if (cmdCount > 0)
    {
        int i;
        printf("\n  -----------------------------------------\n  Post-Installation:\n\n");
        for (i = 0; i < cmdCount; i++) printf("  [%d] %s\n", i + 1, cmdDescs[i]);
        printf("\n");
        if (ask("  Would you like to run these commands"))
        {
            chDir(outDir);
            for (i = 0; i < cmdCount; i++)
            {
                int ret;
                printf("\n  Running: %s\n  Command: %s\n", cmdDescs[i], cmdLines[i]);
                if (ask("  Confirm")) { ret = system(cmdLines[i]); printf("  Exit code: %d\n", ret); }
                else printf("  Skipped.\n");
            }
            chDir(cwd);
        }
    }

    if (argc > 1 || useSfx)
    {
        if (ask(useSfx ? "  Delete installer" : "  Delete archive"))
        {
            if (useSfx) { remove(argv[0]); printf("  Installer deleted.\n"); }
            else { remove(archive); printf("  Deleted: %s\n", archive); }
        }
    }

    if (useSfx) remove(sfxTmp);
    return 0;
}
