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
    
    public:
    // Construtor
    // OperationSupplier();
    // Fornece as instruções e seus opcodes, como registrado no arquivo instructions
    auto/* map<string, int> */ supply_instructions();
    // Fornece as diretivas e suas rotinas, como especificado no arquivo cpp
    auto/* map<string, function> */ supply_directives();
    // Fornece as diretivas de préprocessamento e suas rotinas, como especificado no arquivo cpp
    auto supply_pre_directives() -> std::map<std::string, void(*)(std::vector<asm_line>::iterator&, Preprocesser*)>;
};

#endif