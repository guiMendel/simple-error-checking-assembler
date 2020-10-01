#include <iostream>
#include <fstream>
#include "../include/operation_supplier.hpp"

using namespace std;

#define EVERY_LABEL_IN(line) (const string label : line.labels)
#define NOT_EMPTY(thing) (!thing.empty())

auto OperationSupplier::supply_instructions() -> map<string, int[2]> {
    fstream data_source("data/instructions.txt");
    
    if (!data_source.is_open()) {
        throw invalid_argument("Não foi possível abrir o arquivo de instruções em \"data/instructions.txt\"");
    }

    map<string, int[2]> instruction_table;

    string line;
    int line_number = 0;
    // Para cada linha do arquivo, gera uma entrada na tabela
    try {
        while (getline(data_source, line)) {
            line_number++;

            // Comentários
            if (line[0] == '#') continue;
            size_t opcode_mark = line.find(':');
            size_t size_mark = line.find('>');

            string instruction = line.substr(0, opcode_mark);
            int opcode = stoi(line.substr(opcode_mark + 1, size_mark));
            int size = stoi(line.substr(size_mark + 1));

            instruction_table[instruction][0] = opcode;
            instruction_table[instruction][1] = size;
        }
    }
    catch (invalid_argument error) {
        throw invalid_argument("Entrada inválida no arquivo de instruções, na linha " + to_string(line_number) + ": " + error.what());
    }

    return instruction_table;
}

auto OperationSupplier::supply_directives() -> map<string, void(*)(vector<asm_line>::iterator&, int&)>{
    // Popula a tabela de diretivas
    // Implementação do padrão de projeto Command
    map<string, void(*)(vector<asm_line>::iterator&, int&)> directive_table;
    
    directive_table["SPACE"] = &eval_SPACE;
    directive_table["CONST"] = &eval_CONST;
    
    return directive_table;
}

auto OperationSupplier::supply_pre_directives() -> map<string, void(*)(vector<asm_line>::iterator&, Preprocesser*)> {
    // Popula a tabela de diretivas de préprocessamento
    // Implementação do padrão de projeto Command
    map<string, void(*)(vector<asm_line>::iterator&, Preprocesser*)> pre_directive_table;
    
    pre_directive_table["EQU"] = &eval_EQU;
    pre_directive_table["IF"] = &eval_IF;
    
    return pre_directive_table;
}

// DIRETIVAS DE PREPROCESSAMENTO

void OperationSupplier::eval_EQU(vector<asm_line>::iterator& line_iterator, Preprocesser *pre_instance) {
    const asm_line line = *line_iterator;
    bool verbose = pre_instance->is_verbose();
    map<string, int> &synonym_table = pre_instance->get_synonym_table();

    // Descobre o valor da definição
    int value;
    try {
        value = stoi(line.operand[0]);
    }
    // Se o operando for outro rótulo, stoi() lançará uma exceção
    catch (...) {
        // Verifica se é que havia um operando
        if (line.operand[0].empty()) {
            const MounterException error (-1, "sintático",
                "A diretiva EQU recebe exatamente um parâmetro"
            );
            throw error;
        }
        // Aponta erro, não deveria receber um rótulo
        string att;
        for(auto it = synonym_table.cbegin(); it != synonym_table.cend(); ++it) {
            att += it->first + ": " + to_string(it->second) + "\n";
        }
        att = (att == "" ? "Nenhuma registrada" : att.substr(0, att.length()-1));

        const MounterException error (-1, "semântico",
            "Rótulo \"" + line.operand[0] + "\" não foi atribuído por um EQU antes de ser utilizado por diretiva de pré-processamento.\nAtribuições:\n" + att
        );
        throw error;
    }
    if (verbose) {
        cout << "[" << __FILE__ << "]> Encontrado EQU. Definindo os rótulos ";
        for EVERY_LABEL_IN(line) cout << "\"" << label << "\" ";
        cout << " como " << value << "...";
    }
    for EVERY_LABEL_IN(line) {
        // Verifica por rótulos repetidos
        if (synonym_table.find(label) != synonym_table.end()) {
            throw MounterException(line.number, "semântico",
                string("Redefinição do rótulo \"" + label + "\"")
            );
        }
        synonym_table[label] = value;
    }
    if (verbose) cout << "OK" << endl;
}

void OperationSupplier::eval_IF(vector<asm_line>::iterator& line_iterator, Preprocesser *pre_instance) {
    asm_line &line = *line_iterator;
    bool verbose = pre_instance->is_verbose();

    // Descobre o valor do operando
    int value;
    try {
        value = stoi(line.operand[0]);
    }
    // Se o operando for outro rótulo, stoi() lançará uma exceção
    catch (...) {
        // Verifica se é que havia um operando
        if (line.operand[0].empty()) {
            const MounterException error (-1, "sintático",
                "A diretiva IF recebe exatamente um parâmetro"
            );
            throw error;
        }

        // Aponta erro, não deveria receber um rótulo
        map<string, int> &synonym_table = pre_instance->get_synonym_table();
        string att;
        for(auto it = synonym_table.cbegin(); it != synonym_table.cend(); ++it) {
            att += it->first + ": " + to_string(it->second) + "\n";
        }
        att = (att == "" ? "Nenhuma registrada" : att.substr(0, att.length()-1));
        
        const MounterException error (-1, "semântico",
            "Rótulo \"" + line.operand[0] + "\" não foi atribuído por um EQU antes de ser utilizado por diretiva de pré-processamento.\nAtribuições:\n" + att
        );
        throw error;
    } 
    
    // Executa a regra de negócio
    if (value == 1) {
        if (verbose) cout << "[" << __FILE__ << "]> Encontrado IF avaliado verdadeiro. Mantendo próxima linha" << endl;
    }
    else {
        if (verbose) cout << "[" << __FILE__ << "]> Encontrado IF avaliado falso. Pulando próxima linha" << endl;
        line_iterator++;
    }

    // Move seus rótulos para a linha seguinte
    asm_line &next_line = *(line_iterator + 1);
    next_line.labels.insert(next_line.labels.end(), line.labels.begin(), line.labels.end());
    line.labels.clear();
}

// DIRETIVAS NORMAIS

void OperationSupplier::eval_SPACE(vector<asm_line>::iterator& line_iterator, int& line_number) {
    asm_line &expression = *line_iterator;

    expression.opcode = 0;
    line_number += 1;

    // Certifica o bom uso dos parâmetros
    if (NOT_EMPTY(expression.operand[0])) {
        throw MounterException(expression.number, "sintático",
            "A diretiva SPACE não recebe parâmetros"
        );
    }
}

void OperationSupplier::eval_CONST(vector<asm_line>::iterator& line_iterator, int& line_number) {
    asm_line &expression = *line_iterator;

    // Coloca um 0 preventivo, em caso de exceção
    expression.opcode = 0;
    line_number += 1;

    // Certifica o bom uso dos parâmetros
    if (expression.operand[0].empty() || NOT_EMPTY(expression.operand[1])) {
        throw MounterException(expression.number, "sintático",
            "A diretiva CONST recebe exatamente um parâmetro"
        );
    }

    string operand = expression.operand[0];
    expression.operand[0] = "";

    // Insere a constante no espaço
    int constant;
    try {
        constant = stoi(operand);
    }
    catch (invalid_argument error) {
        throw MounterException(expression.number, "léxico",
            "A diretiva CONST recebe um número como parâmetro. Valor recebido: " + expression.operand[0]
        );
    }
    expression.opcode = constant;
}
