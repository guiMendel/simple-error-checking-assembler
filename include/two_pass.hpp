#ifndef __TWOPASS__
#define __TWOPASS__

#include <map>
#include "../include/scanner.hpp"

class TwoPassAlgorithm {
    // Define se descrções serão impressas
    const bool verbose;
    // Armazena um dicionário das definições de instruções, de instrução para opcode e tamanho
    std::map<std::string, int[2]> instruction_table;
    // Armazena um dicionário das definições de símbolos
    std::map<std::string, int> symbol_table;
    // Dicionário de diretivas de préprocessamento para suas rotinas
    std::map<std::string, void(*)(std::vector<asm_line>::iterator&, int&)> directive_table;

    // Primeira passagem: recebe as linhas do programa e popula a tabela de símbolos
    void first_pass(std::vector<asm_line>&);
    // Segunda passagem: recebe as linhas do programa e gera o uma string que será o conteúdo do arquivo final, pegando os opcodes e passando as labels pela tabela de símbolos
    std::string second_pass(std::vector<asm_line>&);


    public:
    // Recebe um arquivo e cria um novo arquivo .OBJ, com o código montado
    void assemble(std::string, bool print = false);
    // Construtor
    TwoPassAlgorithm(bool verbose = false);
    // Adiciona os rótulos da linha na TS, e adiciona qulquer exceção encontrada no vetor
    void registerLabels(asm_line&, int, std::vector<MounterException>&);
    // Imprime uma linha
    void print_line(asm_line);
};

#endif