#include <regex>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include "../include/scanner.hpp"
#include "../include/mounter_exception.hpp"

using namespace std;

#define OMITABLE true
#define NON_OMITABLE false
#define NOT_EMPTY(thing) !thing.empty()
#define HAS_OPERATION(line) NOT_EMPTY(line.operation)
#define IS_LABEL(token) (token.length()>1) && (token.find(':') == token.length()-1)

vector<asm_line> Scanner::scan (string source_path, string &error_log, bool print/*  = false */) {
    // Lê o arquivo e gera a sua stream
    fstream source(source_path);

    if (!source.is_open()) {
        MounterException error (-1, "null", 
            "Falha ao abrir arquivo \"" + source_path + "\""
        );
        throw error;
    }

    // Início do loop principal
    // Vai receber cada uma das linhas brutas
    string line;
    // Armazena um rótulo que vier em linhas anteriores à sua operação
    vector<string> stray_labels;
    // Armazena a estrutura do programa
    vector<asm_line> program_lines;
    
    for (
        int line_number = 1;
        getline(source, line);
        line_number++
    ) {
        try {
            // Remove o /r da linha
            // line.pop_back();
            // cout << "Line: <" << line << ">" << endl;
            if (line.empty()) continue;
            
            // Separa a linha em elementos
            asm_line broken_line = break_line(line, line_number);

            // Se for uma linha com operação, já registramos
            if (HAS_OPERATION(broken_line)) {
                // Verificamos se há um rótulo declarado anteriormente para essa operação
                assign_labels(broken_line, stray_labels);
                // Registra essa linha de código
                program_lines.push_back(broken_line);
            }
            
            // Se a linha só tiver rótulos, aplicamos eles na linha seguinte
            else if (NOT_EMPTY(broken_line.labels)) {
                stray_labels.insert(stray_labels.end(), broken_line.labels.begin(), broken_line.labels.end());
            }
        }
        catch (ScannerException &error) {
            // Se o erro não for omitível ou o scanner for configurado para reportar todos os erros, adiciona ao log
            if (error.not_omitable() || report_all_errors == true) {
                error_log += "Na linha " + to_string(error.get_line()) + ", erro " + error.get_type() + ": " + error.what() + "\n\n";
            }
            // Constroi o que puder, para que chegue até o fim
            asm_line provisory_line = error.get_provisory_line();
            if (provisory_line.operation != "") {
                // Recebe o mesmo tratamento de rótulo
                assign_labels(provisory_line, stray_labels);
                // Registra essa linha de código
                program_lines.push_back(provisory_line);
            }
        }
        // Batch de exceções da mesma linha
        catch (vector<ScannerException> &batch) {
            // Já coloca a linha provisória no programa
            asm_line provisory_line = batch.at(0).get_provisory_line();
            if (provisory_line.operation != "") {
                program_lines.push_back(provisory_line);
            }

            // Adiciona cada erro ao log
            for (const ScannerException error : batch) {
                // Se o erro não for omitível ou o scanner for configurado para reportar todos os erros, adiciona ao log
                if (error.not_omitable() || report_all_errors == true) {
                    error_log += "Na linha " + to_string(error.get_line()) + ", erro " + error.get_type() + ": " + error.what() + "\n\n";
                }
            }
        }
    }

    // Fecha o arquivo
    source.close();

    if (print) {
        cout << "Estrutura do programa: {" << endl;
        for (const asm_line line : program_lines) {
            cout << "\tLinha " << line.number << ": {";
            string output = "";
            if (NOT_EMPTY(line.labels)) {
                string aux = "labels: [";
                for (const string label : line.labels) {
                    aux += "\"" + label + "\", ";
                }
                output += aux.substr(0, aux.length()-2) + "], ";
            }
            if (HAS_OPERATION(line)) output += "operation: \"" + line.operation + "\", ";
            if (NOT_EMPTY(line.operand[0])) output += "operand1: \"" + line.operand[0] + "\", ";
            if (NOT_EMPTY(line.operand[1])) output += "operand2: \"" + line.operand[1] + "\", ";
            cout << output.substr(0, output.length() - 2) << "}" << endl;
        }
        cout << "}" << endl;
    }

    return program_lines;
}

void Scanner::assign_labels(asm_line &line, vector<string> &stray_labels) {
    // Verificamos se há um rótulo declarado anteriormente para essa operação
    if (NOT_EMPTY(stray_labels)) {
        line.labels.insert(line.labels.end(), stray_labels.begin(), stray_labels.end());
        stray_labels.clear();
    }
}

asm_line Scanner::break_line(string line, int line_number) {
    // cout << "Linha não formatada: \"" << line << "\"" << endl;

    asm_line line_tokens;
    line_tokens.number = line_number;
    line_tokens.operation = "";
    line_tokens.operand[0] = "";
    line_tokens.operand[1] = "";

    // O batch de exceções desta linha. Ao final, uma exceção estática é lançada, que aponta para uma corrente de exeções dinâmicas
    vector<ScannerException> exceptions;

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
        // Erros de rótulo não são omitíveis pois podem alterar o funcionamento das diretivas
        if (IS_LABEL(token)) {
            // Devem ser os primeiros da linha
            if (label_ok) {
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "sintático", NON_OMITABLE, dummy_line,
                    string("Rótulo \"" + token + "\" em posição inválida")
                );
                exceptions.push_back(error);
                continue;
            }

            // Tira os 2 pontos
            token.pop_back();

            // Denuncia tokens inválidos
            if (
                regex_search(token, regex("[^A-Z0-9_]")) ||
                regex_match(token.substr(0, 1), regex("[^A-Z_]"))
            ) {
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "léxico", NON_OMITABLE, dummy_line,
                    string("Rótulo \"" + token + "\" é inválido")
                );
                exceptions.push_back(error);
                continue;
            }

            line_tokens.labels.push_back(token);
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
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                    string("Operação \"" + token + "\" é inválida")
                );
                exceptions.push_back(error);
                continue;
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
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                    string("Operando \"" + token + "\" é inválido")
                );
                exceptions.push_back(error);
                continue;
            }

            operand1_ok = true;
            line_tokens.operand[0] = token;
            continue;
        }

        // Verifica se o token tem uma vírgula no início
        if (token[0] == ',') {
            // Não pode ter mais de uma vírgula por linha
            if (comma_ok) {
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "sintático", OMITABLE, dummy_line,
                    string("Mais de uma vírgula na linha")
                );
                exceptions.push_back(error);
                continue;
            };

            comma_ok = true;
            // Se o operando já vier com a vírgula, tudo pronto
            if (token.length() > 1) {
                operand2_ok = true;
                token = token.substr(1);

                // Denuncia tokens inválidos
                if (regex_search(token, regex("[^A-Z0-9_]"))) {
                    asm_line dummy_line;
                    dummy_line.operation = "";
                    ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                        string("Operando \"" + token + "\" é inválido")
                    );
                    exceptions.push_back(error);
                    continue;
                }
                
                line_tokens.operand[1] = token;
                break;
            }
            continue;
        }

        // Somente deve aceitar o segundo operando se tiver verificado uma vírgula
        if (!comma_ok) {
            asm_line dummy_line;
            dummy_line.operation = "";
            ScannerException error (line_number, "sintático", OMITABLE, dummy_line,
                string("Operando \"" + token + "\" não veio separado por vírgula")
            );
            exceptions.push_back(error);
            continue;
        }

        // É o último operando: adicionamos ele e finalizamos
        // Denuncia tokens inválidos
        if (regex_search(token, regex("[^A-Z0-9_]"))) {
            asm_line dummy_line;
            dummy_line.operation = "";
            ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                string("Operando \"" + token + "\" é inválido")
            );
            exceptions.push_back(error);
            continue;
        }
        line_tokens.operand[1] = token;
        operand2_ok = true;
        break;
    }

    // Se tiver verificado um vírgula mas nenhum segundo operando, é erro
    if (comma_ok && !operand2_ok) {
        asm_line dummy_line;
        dummy_line.operation = "";
        ScannerException error (line_number, "sintático", OMITABLE, dummy_line,
            string("Esperava um segundo argumento após vírgula")
        );
        exceptions.push_back(error);
    };

    if (exceptions.empty()) {
        return line_tokens;
    }
    // Finalmente lança as exceções
    else {
        exceptions.at(0).update_provisory_line(line_tokens);
        // Se for só uma, não precisa lançar um batch
        if (exceptions.size() == 1) {
            throw exceptions.at(0);
        }
        throw exceptions;
    }
}
