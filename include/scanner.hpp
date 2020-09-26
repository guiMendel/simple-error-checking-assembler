#ifndef __SCANNER__
#define __SCANNER__


#include <string>
#include <vector>

// Representa uma linha do código separada por elementos
struct asm_line {
    int number;
    std::string label;
    std::string operation;
    std::string operand[2];
};

// Responsável por ler do arquivo fonte e gerar um vetor com as linhas separadas por elemento
class Scanner {
    // Determina se um token é um rótulo
    bool has_label(std::string token) {return token.find(':') != std::string::npos;}
    // Determina se os erros devem ou não ser reportados
    bool report_errors;
    // Separa uma única linha em seus elementos
    asm_line break_line(std::string, int);
    public:
    Scanner(bool report = true) : report_errors(report) {}
    // Recebe um arquivo e retorna a estrutura do programa. Recebe uma opção de imprimir a estrutura resultante ou não
    std::vector<asm_line> scan(std::string, bool print = false);
};

#endif