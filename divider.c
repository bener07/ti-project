#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <tiffio.h>
#include <stdint.h>


void shuffle_range(int *arr, int n) {
    for (int i = 0; i < n; i++) {
        arr[i] = i;
    }

    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}


int filter_dots(const struct dirent* e){
    return strcmp(".", e->d_name) != 0 && strcmp("..", e->d_name) != 0;
}


/* Copia a página atual de 'in' como nova página de 'out'. */
static int copiar_pagina(TIFF *in, TIFF *out) {
    uint32_t w, h;
    uint16_t bps, spp, foto, planar, comp, orient, formato;

    if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &w) ||
        !TIFFGetField(in, TIFFTAG_IMAGELENGTH, &h) ||
        !TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &foto))
        return -1;

    TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &spp);
    TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &planar);
    TIFFGetFieldDefaulted(in, TIFFTAG_COMPRESSION, &comp);
    TIFFGetFieldDefaulted(in, TIFFTAG_ORIENTATION, &orient);
    TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLEFORMAT, &formato);

    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bps);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, foto);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, planar);
    TIFFSetField(out, TIFFTAG_COMPRESSION, comp);
    TIFFSetField(out, TIFFTAG_ORIENTATION, orient);
    TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, formato);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, h);

    unsigned char *buf = malloc((size_t)TIFFScanlineSize(in));
    if (!buf)
        return -1;

    uint16_t planos = (planar == PLANARCONFIG_SEPARATE) ? spp : 1;
    int erro = 0;

    for (uint16_t p = 0; p < planos && !erro; p++)
        for (uint32_t r = 0; r < h; r++)
            if (TIFFReadScanline(in, buf, r, p) < 0 ||
                TIFFWriteScanline(out, buf, r, p) < 0) {
                erro = 1;
                break;
            }

    free(buf);
    if (erro)
        return -1;

    return TIFFWriteDirectory(out) ? 0 : -1;
}

int addFiles(char *file_name[], char *dest_name, int n) {
    TIFF *out = TIFFOpen(dest_name, "w");
    if (!out)
        return -1;

    int erro = 0;
    for (int f = 0; f < n && !erro; f++) {
        TIFF *in = TIFFOpen(file_name[f], "r");
        if (!in) {
            fprintf(stderr, "Não consigo abrir %s\n", file_name[f]);
            erro = 1;
            break;
        }

        tdir_t n = TIFFNumberOfDirectories(in);
        for (tdir_t d = 0; d < n && !erro; d++)
            if (!TIFFSetDirectory(in, d) || copiar_pagina(in, out) < 0)
                erro = 1;

        TIFFClose(in);
    }

    TIFFClose(out);
    if (erro)
        remove(dest_name);   /* não deixa um resultado incompleto */
    return erro ? -1 : 0;
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <dir_origem>\n", argv[0]);
        return 1;
    }
    srand(time(NULL));

    struct dirent **files;
    int n = scandir(argv[1], &files, filter_dots, NULL);
    if (n < 0) {
        fprintf(stderr, "Erro ao ler diretório %s.\n", argv[1]);
        return 1;
    }

    int *order = malloc(n * sizeof *order);
    char **names = malloc(n * sizeof *names);
    if (!order || !names) {
        fprintf(stderr, "Problemas de alocação de memória.\n");
        return 1;
    }

    shuffle_range(order, n);
    printf("Shuffled files.\n");

    /* Constrói o caminho completo de cada ficheiro, em memória própria */
    for (int i = 0; i < n; i++) {
        const char *nome = files[order[i]]->d_name;
        size_t len = strlen(argv[1]) + 1 + strlen(nome) + 1;
        names[i] = malloc(len);
        if (!names[i]) {
            fprintf(stderr, "Problemas de alocação de memória.\n");
            return 1;
        }
        snprintf(names[i], len, "%s/%s", argv[1], nome);
        printf("%s\n", names[i]);
    }

    int metade = n / 2;

    addFiles(names, "dev.tiff", metade);
    addFiles(names + metade, "test.tiff", n - metade);

    for (int i = 0; i < n; i++) {
        free(names[i]);
        free(files[i]);
    }
    free(names);
    free(files);
    free(order);
    return 0;
}
