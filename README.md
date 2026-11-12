# Projeto TP2 de TI
Projeto realizado no âmbito da disciplina de TI com o objetivo de analisar e criar um codec lossless para ficheiros de imagens do tipo TIFF (Tag Image File Format).<br>
Índice/Fases do Projeto:
* [1. Fase de investigação](#fase-de-investigação)
  * [Introdução](#fase-de-investigação)
  * [Programa Divider](#programa-divider)
  * [Algoritmos de Compressão](#algoritmos-de-compressão)
* [2. Fase de implementação](#fase-de-implementação)

# Fase de investigação
 Pretende-se a realização de uma análise estatística de ficheiros de imagens do tipo TIFF. Para começar espera-se análisar os diferentes dados de teste. Isto é:
 - Análise da entropia do ficheiro
 - Análise de entropia conjunta das imagens
 - Análise de possíveis modelos matemáticos das imagens
 - Análise de padrões nas imagens.
 **Atenção**: __imagens__ refere-se à constituição binária do ficheiro e não ao efeito visual interpretado por humanos.

 É natural que o passo seguinte seja a pesquisa e investigação de algoritmos de compressão adaptados à nossa amostra de dados daí estar referido que algoritmos foram usados e investigados neste projeto na secção [Algoritmos de Compressão](#algoritmos-compressão)


## Programa Divider
 A pedido do Professor, foi feito um programa para a divisão dos dados de teste (aconselhável criar uma pasta com o nome "datasets" que contenha apenas ficheiros tiff). Para compilar o ficheiro é apenas necessário usar o Makefile disponibilizado no presente repositório, como exemplificado no excerto de código abaixo. O Makefile está configurado para procurar na pasta `datasets` por todos os ficheiros tiff e dividi-los em dois conjuntos. Os de __test__ e os de __dev__, esta divisão é feita aleatóriamente.
```bash
make run
```
 Caso pretenda usar uma pasta própria então basta compilar o ficheiro `divider.c` (o que também pode ser feito com o Makefile) e executar. 
 **Nota**: Os ficheiros de output são **sempre** `dev.tiff` e `test.tiff`. Tal que `dev.tiff` está reservado ao desenvolvimento (para evitar o enviesamento dos dados e resultados do codec) e o `test.tiff` a testes para averiguar a generalização do nosso codec relativamente a diferentes conjuntos de dados.

```bash
make run
./div <folder>
```

## Algoritmos de Compressão
# Fase de Implementação
(Ainda nada)
