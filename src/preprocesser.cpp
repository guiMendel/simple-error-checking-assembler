#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "../include/scanner.hpp"
#include "../include/preprocesser.hpp"

using namespace std;

Preprocesser::Preprocesser() {
    // Popula a tabela de diretivas de préprocessamento
    // Implementação do padrão de projeto Command
    pre_directive_table["EQU"] = &eval_EQU;
    pre_directive_table["IF"] = &eval_IF;
}

int Preprocesser::resolve_synonym(string synonym) {
    const auto synonym_entry = synonym_table.find(synonym);
    if (synonym_entry == synonym_table.end()) {
        string error = "Rótulo \"" + synonym + "\" não foi atribuído por um EQU antes de ser utilizado por diretiva de pré-processamento.\nAtribuições:\n";
        string att = "";
        for(auto it = synonym_table.cbegin(); it != synonym_table.cend(); ++it) {
            att += it->first + ": " + to_string(it->second) + "\n";
        }
        att = (att == "" ? "Nenhuma registrada" : att.substr(0, att.length()));
        throw error + att;
    }
    return synonym_entry->second;
}

void Preprocesser::eval_EQU(std::vector<asm_line>::iterator& line_iterator, Preprocesser *pre_instance) {
    const asm_line line = *line_iterator;

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
    cout << "Found EQU. Defining label \"" << line.label << "\" as " << value << "...";
    pre_instance->synonym_table[line.label] = value;
    cout << (pre_instance->synonym_table[line.label] == value ? "OK" : "FAILED") << endl;
}

void Preprocesser::eval_IF(std::vector<asm_line>::iterator& line_iterator, Preprocesser *pre_instance) {
    const asm_line line = *line_iterator;

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
        cout << "Found IF evaluated to true. Keeping next line" << endl;
    }
    else {
        cout << "Found IF evaluated to false. Skipping next line" << endl;
        line_iterator++;
    }    
}

void Preprocesser::preprocess (string path, bool print/* = false */) {
    // O parâmtero solicita que o scanner não levante erros
    Scanner scanner(false);
    // Gera a estrutura do programa
    vector<asm_line> lines = scanner.scan(path, print);

    // Define o nome do arquivo sem a extensão
    const string pre_name = path.substr(0, path.find('.'));
    const string pre_path = pre_name + ".pre";
    // Arquivo a ser construído
    fstream pre(pre_path, fstream::out);

    if (!pre.is_open()) {
        const string error = "Não foi possível criar o arquivo \"" + pre_path + "\"";
        throw error;
    }

    // Passa por cada linha
    for (auto line_iterator = lines.begin(); line_iterator != lines.end(); line_iterator++) {
        process_line(line_iterator, pre);
    }

    pre.close();
    synonym_table.clear();
}

void Preprocesser::process_line(vector<asm_line>::iterator &line_iterator, fstream &output_file) {
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
        return;
    }
    
    // Imprime a linha no arquivo final
    const string label = line.label != "" ? line.label + ": " : "\t";
    const string operation = line.operation;
    const string operands = line.operand[0] != "" ? (" " + line.operand[0] + (line.operand[1] != "" ? ", " + line.operand[1] : "")) : "";
    const string assembled_line = label + operation + operands;
    output_file << assembled_line << endl;
}

int main(int argc, char *argv[]) {
    // Garante que o uso foi correto
    if (argc < 2 || argc > 3) {
        cout << "ERRO: Número de argumentos inválido.\nPor favor, forneça o caminho do arquivo fonte asm." << endl;
        return -1;
    }

    // cout << argv[1] << endl;

    // Determina se deve ou não imprimir a estrutura do programa
    bool print;
    if (argc > 2 && string(argv[2]) == "--print") {
        cout << argv[2] << endl;
        print = true;
    }    
    else print = false;

    try {
        Preprocesser preprocesser;
        preprocesser.preprocess(argv[1], print);
    }
    catch (char const* error) {
        cerr << "ERRO: Exceção no pré-processador. Mensagem:\n" << error << endl;
    }
    catch (string error) {
        cerr << "ERRO: Exceção no pré-processador. Mensagem:\n" << error << endl;
    }

    return 0;
}
