#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "../include/scanner.hpp"
#include "../include/preprocesser.hpp"
#include "../include/mounter_exception.hpp"
#include "../include/operation_supplier.hpp"

using namespace std;

Preprocesser::Preprocesser(bool verbose/* = false */) : verbose(verbose)  {
    // Popula a tabela de diretivas de préprocessamento
    // Implementação do padrão de projeto Command
    // pre_directive_table["EQU"] = &eval_EQU;
    // pre_directive_table["IF"] = &eval_IF;
    OperationSupplier supplier;
    pre_directive_table = supplier.supply_pre_directives();
}

int Preprocesser::resolve_synonym(string synonym) {
    const auto synonym_entry = synonym_table.find(synonym);
    if (synonym_entry == synonym_table.end()) {
        string att = "";
        for(auto it = synonym_table.cbegin(); it != synonym_table.cend(); ++it) {
            att += it->first + ": " + to_string(it->second) + "\n";
        }
        att = (att == "" ? "Nenhuma registrada" : att.substr(0, att.length()-1));
        
        const MounterException error (-1, "semântico",
            "Rótulo \"" + synonym + "\" não foi atribuído por um EQU antes de ser utilizado por diretiva de pré-processamento.\nAtribuições:\n" + att
        );
        throw error;
    }
    return synonym_entry->second;
}

void Preprocesser::preprocess (string path, bool print/* = false */) {
    // O parâmtero solicita que o scanner não levante erros
    Scanner scanner(false);
    // Coleta os erros lançados
    string error_log = "";
    // Gera a estrutura do programa
    vector<asm_line> lines = scanner.scan(path, error_log, print);

    // Define o nome do arquivo sem a extensão
    const string pre_name = path.substr(0, path.find('.'));
    const string pre_path = pre_name + ".pre";
    // Arquivo a ser construído
    fstream pre(pre_path, fstream::out);

    if (!pre.is_open()) {
        const MounterException error(-1, "null",
            "Não foi possível criar o arquivo \"" + pre_path + "\""
        );
        throw error;
    }

    // Coleta as linhas resultantes
    string output_lines = "";

    // Passa por cada linha
    for (auto line_iterator = lines.begin(); line_iterator != lines.end(); line_iterator++) {
        try {
            const string new_line = process_line(line_iterator);
            output_lines += (new_line.empty() ? "" : new_line + "\n");
        }
        catch (MounterException error) {
            // Coleta informações sobre o erro
            const int line = (error.get_line() == -1 ? line_iterator->number : error.get_line());

            error_log += "Na linha " + to_string(line) + ", erro " + error.get_type() + ": " + error.what() + "\n\n";
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
            string(__FILE__) + ":" + to_string(__LINE__) + "> ERRO:\n" + error_log.substr(0, error_log.length()-2)
        );
        throw error;
    }
    
    synonym_table.clear();
}

string Preprocesser::process_line(vector<asm_line>::iterator &line_iterator) {
    const asm_line line = *line_iterator;

    // cout << "Tabela de sinônimos:\n";
    // for(auto it = synonym_table.cbegin(); it != synonym_table.cend(); ++it) {
    //     cout << it->first << ": " << to_string(it->second) << endl;
    // }

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
    string labels;
    for (const string label : line.labels) {
        labels += label + ":\n";
    }
    labels += "\t";
    const string operation = line.operation;
    const string operands = line.operand[0] != "" ? (" " + line.operand[0] + (line.operand[1] != "" ? ", " + line.operand[1] : "")) : "";
    const string assembled_line = labels + operation + operands;
    return assembled_line;
}
