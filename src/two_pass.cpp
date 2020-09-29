#include <regex>
#include <iostream>
#include "../include/two_pass.hpp"
#include "../include/operation_supplier.hpp"

using namespace std;

#define VECTOR_ITERATOR(iterator, a_vector) (auto iterator = a_vector.begin(); iterator != a_vector.end(); iterator++)
#define ANY(thing) (!thing.empty())
#define LABEL_ALREADY_DEFINED(label) (symbol_table.find(label) != symbol_table.end())
#define SECTION_TEXT "TEXT"
#define SECTION_DATA "DATA"
#define INCORRECT_SECTION "Operação em seção incorreta"

TwoPassAlgorithm::TwoPassAlgorithm(bool verbose/* = false */) : verbose(verbose) {
    OperationSupplier supplier;
    instruction_table = supplier.supply_instructions();
    directive_table = supplier.supply_directives();

    for VECTOR_ITERATOR(it, instruction_table) {
        cout << it->first << ": " << to_string(it->second[0]) << ", " << to_string(it->second[1]) << endl;
    }
    for VECTOR_ITERATOR(it, directive_table) {
        cout << it->first << endl;
    }
}

void TwoPassAlgorithm::assemble(std::string path, bool print/* = false */) {
    // O parâmtero solicita que o scanner levante erros
    Scanner scanner(true);
    // Coleta os erros lançados
    string error_log = "";
    // Gera a estrutura do programa
    vector<asm_line> lines = scanner.scan(path, error_log, print);

    // Define o nome do arquivo sem a extensão
    const string obj_name = path.substr(0, path.find('.'));
    const string obj_path = obj_name + ".obj";

    // Arquivo a ser construído
    fstream obj(obj_path, fstream::out);
    if (!obj.is_open()) {
        throw invalid_argument("Não foi possível criar o arquivo \"" + obj_path + "\"");
    }
    obj.close();

    // Primeira passagem
    try {
        first_pass(lines);
    }
    catch (const vector<MounterException> &errors) {
        // Para cada erro
        for (const MounterException error : errors) {
            error_log += "Na linha " + to_string(error.get_line()) + ", erro " + error.get_type() + ": " + error.what() + "\n\n";
        }
    }

    cout << "Estrutura do programa ao final da primeira passagem: {" << endl;
    for (const asm_line line : lines) {
        cout << "\tLinha " << line.number << ": {";
        string output = "";
        if ANY(line.labels) {
            string aux = "labels: [";
            for (const string label : line.labels) {
                aux += "\"" + label + "\", ";
            }
            output += aux.substr(0, aux.length()-2) + "], ";
        }
        output += "opcode: \"" + to_string(line.opcode) + "\", ";
        if ANY(line.operation) output += "operation: \"" + line.operation + "\", ";
        if ANY(line.operand[0]) output += "operand1: \"" + line.operand[0] + "\", ";
        if ANY(line.operand[1]) output += "operand2: \"" + line.operand[1] + "\", ";
        cout << output.substr(0, output.length() - 2) << "}" << endl;
    }
    cout << "}" << endl;

}

void TwoPassAlgorithm::first_pass(vector<asm_line> &lines) {
    // Vai armazenar exceções possíveis para lançar um batch ao final da execução
    vector<MounterException> exceptions;

    // Registra a seção atual
    string current_section = "null";
    // Registra se houve alguma seção texto
    bool section_text_present = false;
    // Ponteiro para a posição do endereço do programa
    int current_line_number = 0;
    // Para cada linha
    for VECTOR_ITERATOR(line_iterator, lines) {
        asm_line expression = *line_iterator;
        // Se houver rótulos
        if ANY(expression.labels) registerLabels(expression, current_line_number, exceptions);

        // Verificação de mudança de seção
        if (expression.operation == "SECTION") {
            string new_section = expression.operand[0];
            
            // Valida a seção
            if (new_section == SECTION_TEXT) {
                section_text_present = true;
            }
            else if (new_section != SECTION_DATA) {
                exceptions.push_back(MounterException(expression.number, "léxico",
                    "Seção \"" + new_section + "\" é inválida. As seções válidas são: " + SECTION_TEXT + ", " SECTION_DATA + ""
                ));
                lines.erase(line_iterator);
                continue;
            }
            current_section = new_section;
            // A seção não chega ao código objeto
            lines.erase(line_iterator);
            continue;
        }

        // Garante que haja algum opcode
        expression.opcode = -1;

        // Verifica a operação nas instruções
        auto instruction_entry = instruction_table.find(expression.operation);
        if (instruction_entry != instruction_table.end()) {
            // Certifica de que está na seção correta
            if (current_section != SECTION_TEXT) {
                exceptions.push_back(MounterException(expression.number, "semântico",
                    INCORRECT_SECTION
                ));
            }

            // Certifica o uso correto dos parâmetros
            int paramaters = (expression.operand[0].empty() ? 0 : 1) + (expression.operand[1].empty() ? 0 : 1);
            int expected_paramteres = instruction_entry->second[1] - 1; // Tamanho da expressão - tamanho da operação
            if (paramaters != expected_paramteres) {
                exceptions.push_back(MounterException(expression.number, "sintático",
                    "Número de parâmetros incorreto para a operação " + expression.operation
                ));
            }

            // Registra o opcde da operação
            expression.opcode = instruction_entry->second[0];
            // Desloca o ponteiro de linha
            current_line_number += instruction_entry->second[1];
            continue;
        }

        // Verifica a operação nas diretivas
        auto directive_entry = directive_table.find(expression.operation);
        if (directive_entry != directive_table.end()) {
            // Certifica de que está na seção correta
            if (current_section != SECTION_DATA) {
                exceptions.push_back(MounterException(expression.number, "semântico",
                    INCORRECT_SECTION
                ));
            }
            // Executa a diretiva e colhe possíveis exceções
            try {
                (*directive_entry->second) (line_iterator, current_line_number);
            }
            catch (const MounterException &error) {
                exceptions.push_back(error);
            }
            continue;
        }

        // Se a operação não é instrução nem diretiva, ela é inválida
        exceptions.push_back(MounterException(expression.number, "léxico",
            "Operação \"" + expression.operation + "\" não identificada"
        ));
    }

    // Certifica de que haja seção texto
    if (!section_text_present) {
        exceptions.push_back(MounterException(-1, "semântico",
            "Seção "s + SECTION_TEXT + " não encontrada"s
        ));
        // Remove as exeções que apontam operações em seção incorreta
        vector<MounterException> filtered_exceptions;
        for VECTOR_ITERATOR(error_iterator, exceptions) {
            if (error_iterator->what() != INCORRECT_SECTION) filtered_exceptions.push_back(*error_iterator);
        }
        exceptions.clear();
        exceptions.insert(exceptions.end(), filtered_exceptions.begin(), filtered_exceptions.end());
    }
    // Reúne os erros de seção incorreta em um só
    else if ANY(exceptions) {
        const string new_message = "As operações das linhas [";
        string error_lines = "";
        vector<MounterException> filtered_exceptions;
        for VECTOR_ITERATOR(error_iterator, exceptions) {
            if (error_iterator->what() == INCORRECT_SECTION) {
                error_lines += to_string(error_iterator->get_line()) + ", ";
            }
            else filtered_exceptions.push_back(*error_iterator);
        }
        if ANY(error_lines) {
            exceptions.clear();
            exceptions.insert(exceptions.end(), filtered_exceptions.begin(), filtered_exceptions.end());
            exceptions.push_back(MounterException(-1, "semântico",
                new_message + error_lines.substr(0, new_message.length()-2) + "] estão em seção incorreta"
            ));
        }
    }

    // Finalmente lança o batch de exceções
    if ANY(exceptions) {
        throw exceptions;
    }
}

void TwoPassAlgorithm::registerLabels(asm_line &expression, int current_line_number, vector<MounterException> &exceptions) {
    for (const string label : expression.labels) {
        // Verifica a validez do rótulo
        if (
            regex_search(label, regex("[^A-Z0-9_]")) ||
            regex_match(label.substr(0,1), regex("[^A-Z_]"))
        ) {
            exceptions.push_back(MounterException(expression.number, "léxico",
                string("Operando \"" + label + "\" é inválido")
            ));
        }
        // Adicionamos à tabela de símbolos, ainda que seja inválido
        // Primeiro verificamos se já tem uma entrada deste rótulo na TS
        if LABEL_ALREADY_DEFINED(label) {
            exceptions.push_back(MounterException(expression.number, "semântico",
                string("Redefinição do rótulo \"" + label + "\". Definição anterior na linha " + to_string(symbol_table[label]))
            ));
            // Fica com a última definição, então prosseguimos
        }
        symbol_table[label] = current_line_number;        
    }
    // Não precisamos mais disso, liberamos a memória
    expression.labels.clear();
}