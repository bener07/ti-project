#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <tiffio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/*  Transforma erros em mensagens que sejam fáceis de compreender,
errno é definido pela "library" "errno.h" */
void dirent_error_messages()
{
    switch (errno) {
        case EACCES:
            printf("Program lacks permissions\n");
            break;
        case EBADF:
            printf("File path does not refer to an open directory stream\n");
            break;
        case EMFILE:
            printf("Program has too many opened files\n");
            break;
        case ENAMETOOLONG:
            printf("Provided path is too long\n");
            break;
        case ENFILE:
            printf("System reached maximum opened files\n");
            break;
        case ENOENT:
            printf("Folder not found\n");
            break;
        case ENOMEM:
            printf("Computer ran out of RAM to proceed with operation\n");
            break;
        case ENOTDIR:
            printf("Path provided is not of a directory\n");
            break;
        // Qualquer outro erro: strerror() transforma o valor de "errno" numa mensagem
        default:
            printf("%s\n", strerror(errno));
            break;
    }
}

// Verifica se o nome do ficheiro tem a extensão ".tiff" ou ".tif"
int is_tiff(const struct dirent * file)
{
    // Retorna 0, se o ficheiro for uma pasta
    if(file->d_type == DT_DIR) { return 0; }
    // Retorna 0, se o nome do ficheiro for "dev.tiff" ou "test.tiff"
    if (strcmp(file->d_name, "dev.tiff") == 0 || strcmp(file->d_name, "test.tiff") == 0) { return 0; }
    // Retorna 0, se o nome do ficheiro não possuir o caractére "."
    char * dot = strrchr(file->d_name, '.');
    if(dot == NULL){ return 0; }
    // Retorna 0, se após (inclusive) o último caractére ".", o resto do nome do ficheiro não for ".tiff" ou ".tif"
    return strcasecmp(dot, ".tif") == 0 ||strcasecmp(dot, ".tiff") == 0;
}

// Organiza numa ordem aleatória os elementos da lista "files"
void shuffle(struct dirent ** files, int num)
{
    for(int i = num - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        struct dirent * temp = files[i];
        files[i] = files[j];
        files[j] = temp;
    }
}

int copy_page(TIFF * in, TIFF * out)
{
    /* Variáveis onde guardamos as propriedades da imagem:
     * - width e height: largura e altura da imagem, em píxeis
     * - bits: número de bits por cada componente de cor de um píxel
     * - samples: número de componentes de cor por píxel (por exemplo, 3 para RGB, 1 para Greyscale)
     * - compression: tipo de compressão usada nos dados da imagem
     * - planar: forma como as componentes de cor estão organizadas
     * - rows_per_strip: número de linhas da imagem que cada "strip" (faixa) contém
     * - photometric: interpretação das cores (preto e branco, RGB, etc.)
     * Os tipos das variáveis têm de ser exatamente os que a "library" "libtiff" usa para cada propriedade */
    uint32_t width, height, rows_per_strip;
    uint16_t bits, samples, compression, planar;
    // Valor por defeito, caso o ficheiro original não indique a interpretação das cores
    uint16_t photometric = PHOTOMETRIC_MINISBLACK;

    // Lemos as propriedades do ficheiro original, sem largura e altura não conseguimos copiar a imagem
    if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &width) ||
        !TIFFGetField(in, TIFFTAG_IMAGELENGTH, &height))
    {
        return 0;
    }
    // "Defaulted": se o ficheiro não tiver a propriedade, a "library" dá-nos o valor por defeito do formato TIFF
    TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bits);
    TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &samples);
    TIFFGetFieldDefaulted(in, TIFFTAG_COMPRESSION, &compression);
    TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &planar);
    TIFFGetFieldDefaulted(in, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);
    TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric);

    // Escrevemos as mesmas propriedades na página do ficheiro de destino
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bits);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, samples);
    TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, planar);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rows_per_strip);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);

    /* Copiamos os píxeis da imagem, uma "strip" de cada vez
     * - strip_size: tamanho (em bytes) de uma "strip" descomprimida
     * - buffer: espaço de memória onde guardamos a "strip" que estamos a copiar */
    tmsize_t strip_size = TIFFStripSize(in);
    unsigned char * buffer = malloc(strip_size);
    if (buffer == NULL)
    {
        return 0;
    }

    int ok = 1;
    int num_strips = TIFFNumberOfStrips(in);
    for (int s = 0; s < num_strips; s++)
    {
        // Lemos a "strip" do ficheiro original (a "library" descomprime-a automaticamente)
        tmsize_t bytes = TIFFReadEncodedStrip(in, s, buffer, strip_size);
        // E escrevemos no ficheiro de destino (a "library" volta a comprimi-la automaticamente)
        // Se a leitura ou a escrita falharem, paramos de copiar esta imagem
        if (bytes < 0 || TIFFWriteEncodedStrip(out, s, buffer, bytes) < 0)
        {
            ok = 0;
            break;
        }
    }

    free(buffer);
    return ok;
}

/* Adiciona todas as páginas do ficheiro TIFF que está em "path" ao ficheiro TIFF "out"
 * Retorna 1 se correu tudo bem, e 0 se houve algum erro. */
int add_file(TIFF * out, const char * path)
{
    // Abrimos o ficheiro original em modo de leitura ("r")
    TIFF * in = TIFFOpen(path, "r");
    if (in == NULL)
    {
        // A "library" "libtiff" já mostra uma mensagem de erro de fácil compreensão
        return 0;
    }

    int ok = 1;
    do
    {
        if (!copy_page(in, out))
        {
            ok = 0;
            break;
        }
        // Termina a página atual do ficheiro de destino, a próxima cópia vai para uma nova página
        TIFFWriteDirectory(out);
    // Passa para a próxima página do ficheiro original, retorna 0 quando não existirem mais páginas
    } while (TIFFReadDirectory(in));

    TIFFClose(in);
    return ok;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    // Para avançar, precisamos do "path" (localização da pasta) da pasta onde estão os ficheiros
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <path_to_directory>\n", argv[0]);
        return 1;
    }

    char * path = argv[1];

    /* - struct dirent: estrutura da "library" "dirent.h", usamo-la para aceder aos ficheiros da pasta
     * - files: lista com os ficheiros ".tiff" da pasta, criada pelo scandir()
     * - num_files: número de ficheiros na lista "files" */
    struct dirent ** files;
    int num_files = scandir(path, &files, is_tiff, NULL);
    if (num_files == -1)
    {
        dirent_error_messages();
        return 1;
    }
    // Precisamos de pelo menos 2 ficheiros, senão um dos ficheiros "TIFF" ficava vazio
    if (num_files < 2)
    {
        printf("A pasta precisa de pelo menos 2 ficheiros '.tiff'\n");
        for (int i = 0; i < num_files; i++) free(files[i]);
        free(files);
        return 1;
    }
    shuffle(files, num_files);

    // Criamos os dois ficheiro "TIFF" onde vamos distribuir e agrupar os ficheiros originais
    TIFF * dev_file = TIFFOpen("dev.tiff", "w");
    TIFF * test_file = TIFFOpen("test.tiff", "w");
    if (dev_file == NULL || test_file == NULL)
    {
        if (dev_file != NULL) { TIFFClose(dev_file); }
        if (test_file != NULL) { TIFFClose(test_file); }
        for (int i = 0; i < num_files; i++) free(files[i]);
        free(files);
        return 1;
    }

    // failed: número de ficheiros originais que não foram copiados com sucesso
    int failed = 0;
    int half = num_files / 2;
    for (int i = 0; i < num_files; i++)
    {
        // O scandir() só nos dá o nome do ficheiro, por isso juntamos o nome da pasta ao nome do ficheiro
        // snprintf() escreve texto formatado para "path", sem nunca ultrapassar o tamanho da variável
        char file_path[1024];
        snprintf(file_path, sizeof file_path, "%s/%s", path, files[i]->d_name);

        // Os ficheiros da primeira metade da lista (já aleatória) vão para o "dev_file", os restantes para o "test_file"
        TIFF * target;
        if (i < half)
        {
            target = dev_file;
        }
        else
        {
            target = test_file;
        }

        // Se a cópia falhar, avisamos e continuamos com o próximo ficheiro
        if (!add_file(target, file_path))
        {
            fprintf(stderr, "Failed to copy %s\n", file_path);
            failed++;
        }
    }

    // Liberta a memória alocada pelo "scandir()"
    for (int i = 0; i < num_files; i++) free(files[i]);
    free(files);

    // O TIFFClose() é que realmente guarda os dados no disco
    TIFFClose(dev_file);
    TIFFClose(test_file);

    // Retorna 1 se algum ficheiro falhou, para quem correu o programa saber que algo correu mal
    if (failed > 0)
    {
        return 1;
    }
    return 0;
}
