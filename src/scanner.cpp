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
#define ANY(thing) (!thing.empty())
#define HAS_OPERATION(line) ANY(line.operation)
#define IS_LABEL(token) (token.length()>1) && (token.find(':') != string::npos)

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
    string stray_label;
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
            if HAS_OPERATION(broken_line) {
                // Verificamos se há um rótulo declarado anteriormente para essa operação
                assign_label(broken_line, stray_label);
                // Registra essa linha de código
                program_lines.push_back(broken_line);
            }
            
            // Se a linha só tiver rótulo, aplicamos ele na linha seguinte
            else if ANY(broken_line.label) {
                // Se já tiver uma armazenada, é erro
                if ANY(stray_label) {
                    stray_label = broken_line.label;
                    asm_line dummy_line;
                    dummy_line.operation = "";
                    throw ScannerException(broken_line.number, "semântico", NON_OMITABLE, dummy_line,
                        string("Mais de um rótulo declarado para a mesma linha")
                    );
                }
                stray_label = broken_line.label;
            }
        }
        catch (ScannerException &error) {
            // Se o erro não for omitível ou o scanner for configurado para reportar todos os erros, adiciona ao log
            if (error.not_omitable() || report_all_errors == true) {
                string intro = (error.get_line() == -1 ? "Erro " : "Na linha " + to_string(error.get_line()) + ", erro ");
                error_log += intro + error.get_type() + ": " + error.what() + "\n";
            }
            // Constroi o que puder, para que chegue até o fim
            asm_line provisory_line = error.get_provisory_line();
            if ANY(provisory_line.operation) {
                // Recebe o mesmo tratamento de rótulo
                try {
                    assign_label(provisory_line, stray_label);
                }
                catch (ScannerException &error) {
                    string intro = (error.get_line() == -1 ? "Erro " : "Na linha " + to_string(error.get_line()) + ", erro ");
                    error_log += intro + error.get_type() + ": " + error.what() + "\n";
                }
                // Registra essa linha de código
                program_lines.push_back(provisory_line);
            }
            // Se a linha só tiver rótulo, aplicamos ele na linha seguinte
            else if ANY(provisory_line.label) {
                // Se já tiver uma armazenada, é erro
                if ANY(stray_label) {
                    stray_label = provisory_line.label;
                    string intro = (provisory_line.number == -1 ? "Erro " : "Na linha " + to_string(provisory_line.number) + ", erro ");
                    error_log += intro + "semântico: Mais de um rótulo declarado para a mesma linha\n";
                }
                stray_label = provisory_line.label;
            }
        }
        // Batch de exceções da mesma linha
        catch (vector<ScannerException> &batch) {
            // Já coloca a linha provisória no programa
            asm_line provisory_line = batch.at(0).get_provisory_line();
            if ANY(provisory_line.operation) {
                // Recebe o mesmo tratamento de rótulo
                try {
                    assign_label(provisory_line, stray_label);
                }
                catch (ScannerException &error) {
                    string intro = (error.get_line() == -1 ? "Erro " : "Na linha " + to_string(error.get_line()) + ", erro ");
                    error_log += intro + error.get_type() + ": " + error.what() + "\n";
                }
                // Registra essa linha de código
                program_lines.push_back(provisory_line);
            }
            // Se a linha só tiver rótulos, aplicamos eles na linha seguinte
            else if ANY(provisory_line.label) {
                // Se já tiver uma armazenada, é erro
                if ANY(stray_label) {
                    stray_label = provisory_line.label;
                    string intro = (provisory_line.number == -1 ? "Erro " : "Na linha " + to_string(provisory_line.number) + ", erro ");
                    error_log += intro + "semântico: Mais de um rótulo declarado para a mesma linha\n";
                }
                stray_label = provisory_line.label;
            }

            // Adiciona cada erro ao log
            for (const ScannerException error : batch) {
                // Se o erro não for omitível ou o scanner for configurado para reportar todos os erros, adiciona ao log
                if (error.not_omitable() || report_all_errors == true) {
                    string intro = (error.get_line() == -1 ? "Erro " : "Na linha " + to_string(error.get_line()) + ", erro ");
                    error_log += intro + error.get_type() + ": " + error.what() + "\n";
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
            if ANY(line.label) output += "label: \"" + line.label + "\", ";
            if HAS_OPERATION(line) output += "operation: \"" + line.operation + "\", ";
            if ANY(line.operand[0]) output += "operand1: \"" + line.operand[0] + "\", ";
            if ANY(line.operand[1]) output += "operand2: \"" + line.operand[1] + "\", ";
            cout << output.substr(0, output.length() - 2) << "}" << endl;
        }
        cout << "}" << endl;
    }

    return program_lines;
}

void Scanner::assign_label(asm_line &line, string &stray_label) {
    // Verificamos se há um rótulo declarado anteriormente para essa operação
    if ANY(stray_label) {
        bool had_label = ANY(line.label);
        line.label = stray_label;
        stray_label = "";
        
        if (had_label) {
            throw ScannerException(line.number, "semântico", NON_OMITABLE, line,
                string("Mais de um rótulo declarado para a mesma linha")
            );
        }
    }
}

asm_line Scanner::break_line(string line, int line_number) {
    // cout << "Linha não formatada: \"" << line << "\"" << endl;

    // Tratamos \t como espaços
    replace(line.begin(), line.end(), '\t', ' ');

    asm_line line_tokens;
    line_tokens.number = line_number;
    line_tokens.opcode = -1;
    line_tokens.label = "";
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

        // Se já tiver lido todos os tokens possíveis (até os 2 operandos), é erro! Esse token não deveria existir
        if (operand2_ok) {
            ScannerException error (line_number, "sintático", NON_OMITABLE, line_tokens,
                string("Token \"" + token + "\" inesperado")
            );
            exceptions.push_back(error);
            break;
        }

        // Tudo em caixa alta
        transform(token.begin(), token.end(), token.begin(), 
            [](unsigned char c) { return toupper(c); }
        );

        // cout << "\tToken: [" << token << ']' << endl;
        
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

            // Denuncia tokens inválidos
            if (
                token[token.length()-1] != ':' ||
                regex_search(token.substr(0, token.length()-1), regex("[^A-Z0-9_]")) ||
                regex_match(token.substr(0, 1), regex("[^A-Z_]"))
            ) {
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "léxico", NON_OMITABLE, dummy_line,
                    string("Rótulo \"" + token + "\" é inválido")
                );
                exceptions.push_back(error);
            }
            else if (token.length() > 50) {
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "léxico", NON_OMITABLE, dummy_line,
                    string("Rótulo \"" + token + "\" excede o limite de 50 caracteres")
                );
                exceptions.push_back(error);
                token = token.substr(0, 51);
            }
            else {
                // Tira os 2 pontos
                token.pop_back();
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
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                    string("Operação \"" + token + "\" é inválida")
                );
                exceptions.push_back(error);
            }

            line_tokens.operation = token;
            operation_ok = true;
            continue;
        }

        // Se ainda não tiver encontrado o operando 1
        if (!operand1_ok) {
            // Verifica se o operando veio com uma vírgula ao final
            if (token[token.length() - 1] == ',') {
                // cout << "<" << token << ">" << endl;
                // Verifica se veio só a vírgula
                if (token[0] == ',') {
                    asm_line dummy_line;
                    dummy_line.operation = "";
                    ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                        string("Operando \"" + token + "\" é inválido")
                    );
                    exceptions.push_back(error);
                    // Adicionamos como parâmetro para que o resultado seja efetivamente inválido e um erro seja eventualmente lecantado
                    operand1_ok = true;
                    line_tokens.operand[0] = token;
                    continue;
                }
                comma_ok = true;
                token.pop_back();
            }
            // Se não, não deve haver um segundo operando
            else {
                operand2_ok = true;
            }

            // Denuncia tokens inválidos
            if (regex_search(token, regex("[^A-Z0-9_-]"))) {
                asm_line dummy_line;
                dummy_line.operation = "";
                ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                    string("Operando \"" + token + "\" é inválido")
                );
                exceptions.push_back(error);
            }

            operand1_ok = true;
            line_tokens.operand[0] = token;
            continue;
        }

        // É o último operando
        // Denuncia tokens inválidos
        if (regex_search(token, regex("[^A-Z0-9_-]"))) {
            asm_line dummy_line;
            dummy_line.operation = "";
            ScannerException error (line_number, "léxico", OMITABLE, dummy_line,
                string("Operando \"" + token + "\" é inválido")
            );
            exceptions.push_back(error);
        }
        line_tokens.operand[1] = token;
        operand2_ok = true;
        // Agora temos que nos certificar de que o próximo token comece com um comentário
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
