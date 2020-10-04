#ifndef __SCANNER__
#define __SCANNER__

#include <string>
#include <vector>
#include "mounter_exception.hpp"

// Representa uma linha do código separada por elementos
struct asm_line {
    // Indica qual a linha no arquivo fonte .asm
    int number;
    // std::vector<std::string> labels;
    std::string label;
    std::string operation;
    std::string operand[2];
    // // Indica qual será a linha no arquivo final .obj
    // int final_number;
    // Indica qual o código opcode da instrução
    int opcode;
};

// Uma especificação das exceções de montador que servem uma linha provisória
class ScannerException : public MounterException {
    // Define se é uma reportagem omitível
    const bool omitable;
    // Define uma construção de linha provisória, que é construída apenas para que o programa continue seja possível levantar outros possíveis erros em linhas seguintes
    asm_line provisory_line;

    public:
    ScannerException(int line, std::string type, bool omitable, asm_line provisory_line, std::string message) :
        MounterException(line, type, message),
        omitable(omitable),
        provisory_line(provisory_line)
        {}

    const bool not_omitable() const {return !omitable;}
    asm_line get_provisory_line() const {return provisory_line;}
    void update_provisory_line(asm_line update) {provisory_line = update;}
};

// Responsável por ler do arquivo fonte e gerar um vetor com as linhas separadas por elemento
class Scanner {
    // Determina se os erros devem ou não ser reportados
    bool report_all_errors;
    // Separa uma única linha em seus elementos
    asm_line break_line(std::string, int);
    
    public:
    Scanner(bool report = true) : report_all_errors(report) {}
    // Recebe um arquivo e retorna a estrutura do programa. Recebe uma opção de imprimir a estrutura resultante ou não. Recebe uma referência string na qual imprime todos os erros encontrados.
    std::vector<asm_line> scan(std::string, std::string&, bool print = false);
    // Recebe uma linha e um vetor de rótulos, e encaixa os rótulos na linha.
    void assign_label(asm_line&, std::string&);
};

#endif