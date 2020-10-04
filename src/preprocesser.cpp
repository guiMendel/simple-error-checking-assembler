#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "../include/scanner.hpp"
#include "../include/preprocesser.hpp"
#include "../include/mounter_exception.hpp"
#include "../include/operation_supplier.hpp"

#define NOT_EMPTY(thing) (!thing.empty())

using namespace std;

Preprocesser::Preprocesser(bool verbose/* = false */) : verbose(verbose)  {
    // Popula a tabela de diretivas de préprocessamento
    // Implementação do padrão de projeto Command
    // pre_directive_table["EQU"] = &eval_EQU;
    // pre_directive_table["IF"] = &eval_IF;
    OperationSupplier supplier;
    pre_directive_table = supplier.supply_pre_directives();
}

// void* Preprocesser::resolve_synonym(string synonym) {
//     const auto synonym_entry = synonym_table.find(synonym);
//     if (synonym_entry == synonym_table.end()) {
//         string att = "";
//         for(auto it = synonym_table.cbegin(); it != synonym_table.cend(); ++it) {
//             att += it->first + ": " + to_string(it->second) + "\n";
//         }
//         att = (att == "" ? "Nenhuma registrada" : att.substr(0, att.length()-1));
        
        // const MounterException error (-1, "semântico",
        //     "Rótulo \"" + synonym + "\" não foi atribuído por um EQU antes de ser utilizado por diretiva de pré-processamento.\nAtribuições:\n" + att
        // );
        // throw error;
//     }
//     return synonym_entry->second;
// }

void Preprocesser::preprocess (string path, bool print/* = false */) {
    // O parâmtero solicita que o scanner não levante erros
    Scanner scanner(false);
    // Coleta os erros lançados
    string error_log = "";
    // Gera a estrutura do programa
    vector<asm_line> lines = scanner.scan(path, error_log, print);

    // Levanta erro se receber o tipo errado de arquivo
    const size_t dot = path.find('.');
    if (path.substr(dot) != ".asm"s) {
        throw invalid_argument("Tipo de arquivo inválido. Por favor, forneça um arquivo .asm para o modo pré-processamento");
    }
    // Define o nome do arquivo sem a extensão
    const string pre_path = path.substr(0, dot) + ".pre";
    // Arquivo a ser construído
    fstream pre(pre_path, fstream::out);

    if (!pre.is_open()) {
        throw invalid_argument("Não foi possível criar o arquivo \"" + pre_path + "\"");
    }

    // Coleta as linhas resultantes
    string output_lines = "";

    // Passa por cada linha
    for (auto line_iterator = lines.begin(); line_iterator != lines.end(); line_iterator == lines.end() ? line_iterator : line_iterator++) {
        try {
            // cout << "Processando linha " << line_iterator->number << endl;
            const string new_line = process_line(line_iterator);
            output_lines += (new_line.empty() ? "" : new_line + "\n");
        }
        catch (MounterException error) {
            cout << "Erro na linha " << line_iterator->number << " (" << error.what() << ")" << endl;
            // Coleta informações sobre o erro
            const int line = (error.get_line() == -1 ? line_iterator->number : error.get_line());

            error_log += "Na linha " + to_string(line) + ", erro " + error.get_type() + ": " + error.what() + "\n";
        }
    }

    if (verbose) {
        cout << "Definições da tabela de sinônimos:\n";
        for (auto synonym_entry_it = synonym_table.begin(); synonym_entry_it != synonym_table.end(); synonym_entry_it++) {
            cout << "\t" << synonym_entry_it->first << ": " << synonym_entry_it->second << endl;
        }
    }
    
    // Finaliza o arquivo ou imprime os erros
    if (error_log.empty()) {
        pre << output_lines;
        pre.close();
    }
    else {
        pre.close();
        // Deleta o arquivo vazio
        remove(pre_path.c_str());

        MounterException error (-1, "null",
            string(__FILE__) + ":" + to_string(__LINE__) + "> ERRO:\n" + error_log.substr(0, error_log.length()-1)
        );
        throw error;
    }
    
    synonym_table.clear();
}

string Preprocesser::process_line(vector<asm_line>::iterator &line_iterator) {
    asm_line &line = *line_iterator;

    // cout << "Tabela de sinônimos:\n";
    // for(auto it = synonym_table.cbegin(); it != synonym_table.cend(); ++it) {
    //     cout << it->first << ": " << to_string(it->second) << endl;
    // }

    // Substitui ocorrências de sinônimos pelos seus valores
    if NOT_EMPTY(line.operand[0]) {
        auto synonym_entry = synonym_table.find(line.operand[0]);
        if (synonym_entry != synonym_table.end()) {
            line.operand[0] = to_string(synonym_entry->second);
        }
    }
    if NOT_EMPTY(line.operand[1]) {
        auto synonym_entry = synonym_table.find(line.operand[1]);
        if (synonym_entry != synonym_table.end()) {
            line.operand[1] = to_string(synonym_entry->second);
        }
    }

    // Verifica a operação da linha contra as diretivas de préprocessamento
    const auto directive_entry = pre_directive_table.find(line.operation);
    // Verifica se houve correspondência
    if (directive_entry != pre_directive_table.end()) {
        // Invoca a rotina da diretiva
        auto eval_routine = directive_entry->second;
        eval_routine(line_iterator, this);
        // A diretiva não vai para o programa final
        return "";
    }
    
    // Imprime a linha no arquivo final
    string label = NOT_EMPTY(line.label) ? line.label + "\n    " : "    ";
    const string operation = line.operation;
    const string operands = line.operand[0] != "" ? (" " + line.operand[0] + (line.operand[1] != "" ? ", " + line.operand[1] : "")) : "";
    const string assembled_line = label + operation + operands;
    return assembled_line;
}
