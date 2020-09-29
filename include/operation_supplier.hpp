#ifndef __SUPPLIER__
#define __SUPPLIER__

#include <map>
#include <vector>
#include "scanner.hpp"
#include "preprocesser.hpp"

class OperationSupplier {
    // DIRETIVAS PRÉPROCESSAMENTO
    // Executa a diretiva EQU
    static void eval_EQU(std::vector<asm_line>::iterator&, Preprocesser*);
    // Executa a diretiva IF
    static void eval_IF(std::vector<asm_line>::iterator&, Preprocesser*);

    // DIRETIVAS
    // Executa a diretiva SPACE
    static void eval_SPACE(std::vector<asm_line>::iterator&, int&);
    // Executa a diretiva CONST
    static void eval_CONST(std::vector<asm_line>::iterator&, int&);
    
    public:
    // Fornece as instruções e seus opcodes, como registrado no arquivo instructions
    auto supply_instructions() -> std::map<std::string, int[2]>;
    // Fornece as diretivas e suas rotinas, como especificado no arquivo cpp
    auto supply_directives() -> std::map<std::string, void(*)(std::vector<asm_line>::iterator&, int&)>;
    // Fornece as diretivas de préprocessamento e suas rotinas, como especificado no arquivo cpp
    auto supply_pre_directives() -> std::map<std::string, void(*)(std::vector<asm_line>::iterator&, Preprocesser*)>;
};

#endif