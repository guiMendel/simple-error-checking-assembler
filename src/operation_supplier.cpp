#include <iostream>
#include "../include/operation_supplier.hpp"

using namespace std;

#define EVERY_LABEL_IN(line) (const string label : line.labels)

auto OperationSupplier::supply_pre_directives() -> std::map<std::string, void(*)(std::vector<asm_line>::iterator&, Preprocesser*)> {
    // Popula a tabela de diretivas de préprocessamento
    // Implementação do padrão de projeto Command
    std::map<std::string, void(*)(std::vector<asm_line>::iterator&, Preprocesser*)> pre_directive_table;
    
    pre_directive_table["EQU"] = &eval_EQU;
    pre_directive_table["IF"] = &eval_IF;
    
    return pre_directive_table;
}

void OperationSupplier::eval_EQU(std::vector<asm_line>::iterator& line_iterator, Preprocesser *pre_instance) {
    const asm_line line = *line_iterator;
    bool verbose = pre_instance->is_verbose();
    std::map<std::string, int> &synonym_table = pre_instance->get_synonym_table();

    // Descobre o valor da definição
    int value;
    try {
        value = stoi(line.operand[0]);
    }
    // Se o operando for outro rótulo, stoi() lançará uma exceção
    catch (invalid_argument error) {
        // Verifica se o rótulo foi definido anteriormente por outro EQU
        value = pre_instance->resolve_synonym(line.operand[0]);
    }
    if (verbose) {
        cout << "[" << __FILE__ << "]> Encontrado EQU. Definindo os rótulos ";
        for EVERY_LABEL_IN(line) cout << "\"" << label << "\" ";
        cout << " como " << value << "...";
    }
    for EVERY_LABEL_IN(line) synonym_table[label] = value;
    if (verbose) cout << "OK" << endl;
}

void OperationSupplier::eval_IF(std::vector<asm_line>::iterator& line_iterator, Preprocesser *pre_instance) {
    const asm_line line = *line_iterator;
    bool verbose = pre_instance->is_verbose();

    // Descobre o valor do operando
    int value;
    try {
        value = stoi(line.operand[0]);
    }
    // Se o operando for outro rótulo, stoi() lançará uma exceção
    catch (invalid_argument error) {
        // Verifica se o rótulo foi definido anteriormente por outro EQU
        value = pre_instance->resolve_synonym(line.operand[0]);
    }    
    
    // Executa a regra de negócio
    if (value == 1) {
        if (verbose) cout << "[" << __FILE__ << "]> Encontrado IF avaliado verdadeiro. Mantendo próxima linha" << endl;
    }
    else {
        if (verbose) cout << "[" << __FILE__ << "]> Encontrado IF avaliado falso. Pulando próxima linha" << endl;
        line_iterator++;
    }    
}
