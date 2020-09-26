#include <regex>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>

using namespace std;

struct asm_line {
    int number;
    string label;
    string operation;
    string operand[2];
};

class Mounter {
    // Responsável por ler do arquivo fonte e gerar uma lista de linhas separadas em elementos
    fstream source;
    // Vetor de linhas divididas do código
    vector<asm_line> program_lines;
    // Separa uma linha do código nos elementos rótulo, operação e operandos
    asm_line break_line(string, int);
    // Determina se um token é um rótulo
    bool has_label(string token) {return token.find(':') != string::npos;}
    void populate_lines();
    public:
    Mounter(fstream&);
    ~Mounter() {source.close();}
    void print_list();
};

Mounter::Mounter (fstream &provided_source) {
    if (!provided_source.is_open()) {
        throw "Stream de arquivo não estava aberta. Favor, fornceça uma stream com arquivo aberto.";
    }
    source.swap(provided_source);
    populate_lines();
}

asm_line Mounter::break_line(string line, int number) {
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

void Mounter::populate_lines () {
    // Inicia o loop principal linha por linha
    string line;
    // Posiciona o ponteiro no início da stream
    source.seekg(0);
    // Armazena um rótulo que vieram em linhas anteriores à sua operação
    string stray_label;
    int i = 1;
    while (getline(source, line)) {
        // Remove o /r da linha
        // line.pop_back();
        // cout << "Line: <" << line << ">" << endl;
        i++;
        if (line.empty()) continue;
        
        // Separa a linha em elementos
        asm_line broken_line = break_line(line, i-1);

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
}

void Mounter::print_list () {
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

// Recebe o caminho do arquivo fonte asm como argumento
int main(int argc, char *argv[]) {
    // Garante que o uso foi correto
    if (argc != 2) {
        cout << "ERRO: Número de argumentos inválido.\nPor favor, forneça o caminho do arquivo fonte asm." << endl;
        return -1;
    }

    // cout << argv[1] << endl;

    // Acessamos o arquivo fonte
    fstream source(argv[1]);
    // Confirmamos o sucesso do acesso
    if (source.is_open()) {
        try {
            Mounter mounter(source);
            mounter.print_list();
        }
        catch (char const* error) {
            cerr << "ERRO: Exceção no montador. Mensagem:\n" << error << endl;
        }
        catch (string error) {
            cerr << "ERRO: Exceção no montador. Mensagem:\n" << error << endl;
        }
    }

    else cout << "ERRO: Falha ao abrir arquivo " << argv[1] << endl;
    

    return 0;
}
