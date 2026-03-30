#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned char buf[4096];
    size_t n, total = 0;
    int col;

    if (argc < 3) { fprintf(stderr, "Usage: MasonGenStub input.exe output.h\n"); return 1; }

    fin = fopen(argv[1], "rb");
    if (!fin) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }

    fout = fopen(argv[2], "w");
    if (!fout) { fclose(fin); return 1; }

    fseek(fin, 0, SEEK_END);
    total = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    fprintf(fout, "static const unsigned char MasonStubBytes[] = {\n");

    col = 0;
    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0) {
        size_t i;
        for (i = 0; i < n; i++) {
            if (col == 0) fprintf(fout, "    ");
            fprintf(fout, "0x%02X", buf[i]);
            total--;
            if (total > 0 || i + 1 < n) fprintf(fout, ",");
            col++;
            if (col >= 16) { fprintf(fout, "\n"); col = 0; }
        }
    }

    if (col > 0) fprintf(fout, "\n");
    fprintf(fout, "};\n");
    fprintf(fout, "static const unsigned long MasonStubSize = sizeof(MasonStubBytes);\n");

    fclose(fin);
    fclose(fout);
    return 0;
}
