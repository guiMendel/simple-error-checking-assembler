#include <regex>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include "../include/scanner.hpp"

using namespace std;

vector<asm_line> Scanner::scan (string source_path, bool print/*  = false */) {
    // Lê o arquivo e gera a sua stream
    fstream source(source_path);

    if (!source.is_open()) {
        string error = "Falha ao abrir arquivo \"" + source_path + "\"";
        throw error;
    }

    // Início do loop principal
    // Vai receber cada uma das linhas brutas
    string line;
    // Armazena um rótulo que vier em linhas anteriores à sua operação
    string stray_label;
    // Armazena a estrutura do programa
    vector<asm_line> program_lines;
    
    for (
        int line_number = 1;
        getline(source, line);
        line_number++
    ) {
        // Remove o /r da linha
        // line.pop_back();
        // cout << "Line: <" << line << ">" << endl;
        if (line.empty()) continue;
        
        // Separa a linha em elementos
        asm_line broken_line = break_line(line, line_number);

        // Se for uma linha completa, já registramos
        if (broken_line.operation != "") {
            // Verificamos se há um rótulo declarado anteriormente para essa operação
            if (stray_label != "") {
                // Garantimos que não haja outro rótulo nessa mesma linha
                if (broken_line.label != "") {
                    string error = "Rótulo \"" + broken_line.label + "\" conflita com o rótulo declarado anteriormente \"" + stray_label + "\"";
                    throw error;
                }

                broken_line.label = stray_label;
                // Limpamos o cache de rótulo
                stray_label = "";
            }
            // Registra essa linha de código
            program_lines.push_back(broken_line);
        }
        
        // Se a linha só tiver um rótulo, aplicamos ele na linha seguinte
        else if (broken_line.label != "") {
            // Se já tiver um rótulo armazenado, levanta um erro
            if (stray_label != "") {
                string error = "Rótulo \"" + broken_line.label + "\" conflita com o rótulo declarado anteriormente \"" + stray_label + "\"";
                throw error;
            }
            stray_label = broken_line.label;
        }
    }

    // Fecha o arquivo
    source.close();

    if (print) {
        cout << "Estrutura do programa: {" << endl;
        for (const asm_line line : program_lines) {
            cout << "\tLinha " << line.number << ": {";
            string aux = "";
            if (line.label.length()) aux += "label: \"" + line.label + "\", ";
            if (line.operation.length()) aux += "operation: \"" + line.operation + "\", ";
            if (line.operand[0].length()) aux += "operand1: \"" + line.operand[0] + "\", ";
            if (line.operand[1].length()) aux += "operand2: \"" + line.operand[1] + "\", ";
            cout << aux.substr(0, aux.length() - 2) << "}" << endl;
        }
        cout << "}" << endl;
    }

    return program_lines;
}

asm_line Scanner::break_line(string line, int number) {
    // cout << "Linha não formatada: '" << line << "'" << endl;

    asm_line line_tokens;
    line_tokens.number = number;
    line_tokens.label = "";
    line_tokens.operation = "";
    line_tokens.operand[0] = "";
    line_tokens.operand[1] = "";

    stringstream ss(line);
    // Vamos ler cada token e colocá-lo em seu lugar
    string token;
    // string formatted_line = "";

    // Indica se um rótulo foi adicionado
    bool label_ok = false;
    // Indica se já adicionou a operação
    bool operation_ok = false;
    // Indica se já adicionou o operando 1
    bool operand1_ok = false;
    // Indica se verificou a presença da vírgula após o primeiro operando
    bool comma_ok = false;
    // Indica se já adicionou o operando 2
    bool operand2_ok = false;

    // Indica se já encontrou todos os elementos necessários
    bool finished = false;

    while (!finished && getline(ss, token, ' ')) {
        // Descarta os vazios
        if (token.empty()) continue;

        // Verifica a existência do char ';', que indica início de comentário
        size_t position = token.find(';');
        // Se houver
        if (position != string::npos) {
            // Verifica se há um token antes do comentário
            if (position > 0) {
                // Se houver, seleciona apenas esse token
                token = token.substr(0, position);
                // Indica que esta será a última iteração
                finished = true;
            }
            else {
                // Se não, já pula fora do loop
                break;
            }
        }

        // Tudo em caixa alta
        transform(token.begin(), token.end(), token.begin(), 
            [](unsigned char c) { return toupper(c); }
        );

        // cout << '[' << token << ']' << endl;
        
        // Caso seja um rótulo
        if (has_label(token)) {
            // Se não for a primeira palavra, é erro
            if (label_ok) {
                string error = "Rótulo '" + token + "' inválido";
                throw error;
            }

            // Tira os 2 pontos
            token.pop_back();

            // Denuncia tokens inválidos
            if (
                regex_search(token, regex("[^A-Z0-9_]")) ||
                regex_match(token.substr(0, 1), regex("[^A-Z_]"))
            ) {
                string error = "Token '" + token + "' é inválido";
                throw error;
            }

            line_tokens.label = token;
            label_ok = true;
            continue;
        }
        label_ok = true;

        // Se ainda não tiver encontrado uma operação
        if (!operation_ok) {
            // Denuncia tokens inválidos
            if (
                regex_search(token, regex("[^A-Z0-9_]")) ||
                regex_match(token.substr(0, 1), regex("[^A-Z_]"))
            ) {
                string error = "Operação '" + token + "' é inválida";
                throw error;
            }

            line_tokens.operation = token;
            operation_ok = true;
            continue;
        }

        // Se ainda não tiver encontrado o operando 1
        if (!operand1_ok) {
            // Verifica se o operando veio com uma vírgula ao final
            if (token[token.length() - 1] == ',') {
                comma_ok = true;
                token.pop_back();
            }

            // Denuncia tokens inválidos
            if (regex_search(token, regex("[^A-Z0-9_]"))) {
                string error = "Operando '" + token + "' é inválido";
                throw error;
            }

            operand1_ok = true;
            line_tokens.operand[0] = token;
            continue;
        }

        // Verifica se o token tem uma vírgula no início
        if (token[0] == ',') {
            // Não pode ter mais de uma vírgula por linha
            if (comma_ok) throw "Mais de uma vírgula na linha";

            comma_ok = true;
            // Se o operando já vier com a vírgula, tudo pronto
            if (token.length() > 1) {
                operand2_ok = true;
                token = token.substr(1);

                // Denuncia tokens inválidos
                if (regex_search(token, regex("[^A-Z0-9_]"))) {
                    string error = "Operando '" + token + "' é inválido";
                    throw error;
                }
                
                line_tokens.operand[1] = token;
                break;
            }
            continue;
        }

        // Somente deve aceitar o segundo operando se tiver verificado uma vírgula
        if (!comma_ok) {
            string error = "Operando '" + token + "' não veio separado por vírgula";
            throw error;
        }

        // É o último operando: adicionamos ele e finalizamos
        // Denuncia tokens inválidos
        if (regex_search(token, regex("[^A-Z0-9_]"))) {
            string error = "Operando '" + token + "' é inválido";
            throw error;
        }
        line_tokens.operand[1] = token;
        operand2_ok = true;
        break;
    }

    // Se tiver verificado um vírgula mas nenhum segundo operando, é erro
    if (comma_ok && !operand2_ok) throw "Esperava um segundo argumento após vírgula";

    return line_tokens;
}

// int main(int argc, char *argv[]) {
//     // Garante que o uso foi correto
//     if (argc != 2) {
//         cout << "ERRO: Número de argumentos inválido.\nPor favor, forneça o caminho do arquivo fonte asm." << endl;
//         return -1;
//     }

//     // cout << argv[1] << endl;

//     try {
//         Scanner scanner;
//         scanner.scan(argv[1], true);
//     }
//     catch (char const* error) {
//         cerr << "ERRO: Exceção no pré-processador. Mensagem:\n" << error << endl;
//     }
//     catch (string error) {
//         cerr << "ERRO: Exceção no pré-processador. Mensagem:\n" << error << endl;
//     }

//     return 0;
// }
