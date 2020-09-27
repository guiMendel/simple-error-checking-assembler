#include <iostream>
#include "include/preprocesser.hpp"

using namespace std;

int main(int argc, char *argv[]) {
    // Garante que o uso foi correto
    if (argc < 3 || argc > 4) {
        cerr << "ERRO: Número de argumentos inválido.\nPor favor, forneça o tipo de compilação: -p para preprocessar um arquivo .asm e -o para montar um arquivo .pre.\nForneça também o caminho par ao arquivo." << endl;
        return -1;
    }

    // cout << argv[1] << endl;

    // Analise os parâmetros
    vector<char*> args (argv + 1, argv + argc);
    // Define se a árvore do programa deve ser impressa
    bool print;
    // Define se ocorrerá montagem ou préprocessamento
    string mode = "";
    // Guardará o caminho do arquivo fonte
    string source_file_path = "";

    try {
        // Para cada argumento
        for (const char* carg : args) {
            // Transforma em string
            string arg = string(carg);
            
            if      (arg == "--print") {
                print = true;
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
        cerr << "ERRO: " << error << "\nPor favor, forneça o tipo de compilação: -p para preprocessar um arquivo .asm e -o para montar um arquivo .pre.\nForneça também o caminho para o arquivo." << endl;
        return -1;
    }
    catch (string error) {
        cerr << "ERRO: " << error << "\nPor favor, forneça o tipo de compilação: -p para preprocessar um arquivo .asm e -o para montar um arquivo .pre.\nForneça também o caminho para o arquivo." << endl;
        return -1;
    }

    try {
        if (mode == "-p") {
            Preprocesser preprocesser;
            preprocesser.preprocess(source_file_path, print);
        }
    }
    catch (exception error) {
        cerr << "ERRO: " << error.what() << endl;
    }

    return 0;
}
