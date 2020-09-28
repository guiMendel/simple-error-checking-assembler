#include <string.h>
#include <iostream>
#include "include/preprocesser.hpp"

using namespace std;

int main(int argc, char *argv[]) {
    // Descrição do uso correto
    const string help = "\
Forneça o tipo de compilação:\n\
-p para preprocessar um arquivo do tipo .asm e gerar um arquivo .pre\n\
-o para montar um arquivo .pre em um arquivo executável\n\
\n\
Forneça também o caminho para o arquivo fonte\n\
\n\
Outras opções:\n\
\t--print: Imprime a estrutura do programa, como construída pelo módulo scanner\n\
\t--verbose: Imprime descrições detalhadas da execução\n\
";
    // Ajuda os necessitados
    // string thing = string(argv[1]);
    // cout << argc << " " << argv[1] << "\n" << typeid(thing).name() << " " << typeid("help").name() << endl;
    if (argc == 2 && (strcmp(argv[1], "help") || strcmp(argv[1], "--help") || strcmp(argv[1], "-h"))) {
        cout << "Bem vindo a este montador básico!\n" << help << endl;
        return 0;
    }

    // Garante que o uso foi correto
    if (argc < 3 || argc > 5) {
        cerr << "ERRO: Número de argumentos inválido.\n" << help << endl;
        return -1;
    }

    // cout << argv[1] << endl;

    // Analise os parâmetros
    vector<char*> args (argv + 1, argv + argc);
    // Define se a árvore do programa deve ser impressa
    bool print = false;
    // Define se imprime descrições
    bool verbose = false;
    // Define se ocorrerá montagem ou préprocessamento
    string mode = "";
    // Guardará o caminho do arquivo fonte
    string source_file_path = "";

    try {
        // Para cada argumento
        for (const char* carg : args) {
            // Transforma em string
            string arg = string(carg);
            // cout << arg << endl;
            
            if      (arg == "--print") {
                print = true;
            }

            else if      (arg == "--verbose") {
                verbose = true;
            }

            else if (arg == "-p" || arg == "-o") {
                if (mode.empty()) mode = arg;
                else throw "Argumentos inválidos.";
            }

            else if (arg[0] != '-') {
                if (source_file_path.empty()) source_file_path = arg;
                else throw "Argumentos inválidos.";
            }

            else {
                throw "Argumentos inválidos.";
            }
        }
        // Se não tiver um modo ou caminho nos argumentos, erro
        if (mode.empty()) {
            throw "Tipo de compilação não especificado.";
        }
        if (source_file_path.empty()) {
            throw "Arquivo fonte não especificado.";
        }
    }
    catch (char const* error) {
        cerr << __FILE__ << ":" << __LINE__ << "> ERRO: " << error << "\n" << help << endl;
        return -1;
    }
    catch (string error) {
        cerr << __FILE__ << ":" << __LINE__ << "> ERRO: " << error << "\n" << help << endl;
        return -1;
    }

    try {
        if (mode == "-p") {
            Preprocesser preprocesser(verbose);
            preprocesser.preprocess(source_file_path, print);
        }
    }
    catch (exception &error) {
        cerr << __FILE__ << ":" << __LINE__ << "> ERRO:\n" << error.what() << endl;
    }

    return 0;
}
