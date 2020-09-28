#ifndef __PREPROCESSER__
#define __PREPROCESSER__

#include <iterator>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "scanner.hpp"

class Preprocesser {
    // Define se descrções serão impressas
    const bool verbose;
    // Armazena um dicionário das definições de sinônimo do programa
    std::map<std::string, int> synonym_table;
    // Dicionário de diretivas de préprocessamento para suas rotinas
    std::map<std::string, void(*)(std::vector<asm_line>::iterator&, Preprocesser*)> pre_directive_table;
    // Processa uma linha, adicionando ela ao arquivo final ou executando uma diretiva de préprocessamento
    std::string process_line(std::vector<asm_line>::iterator&);
    // Tenta acessar o valor atribuído ao parametro pela tabela de sinônimos. Se não houver, lança uma exceção
    int resolve_synonym(std::string synonym);
    // Executa a diretiva EQU
    static void eval_EQU(std::vector<asm_line>::iterator&, Preprocesser*);
    // Executa a diretiva IF
    static void eval_IF(std::vector<asm_line>::iterator&, Preprocesser*);
    public:
    // Recebe um arquivo e cria um novo arquivo .PRE, com o código preprocessado
    void preprocess(std::string, bool print = false);
    // Construtor
    Preprocesser(bool verbose = false);
};

#endif