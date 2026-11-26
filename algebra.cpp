// ============================================================
//                       ALGEBRA.CPP
// Implementação das ferramentas matemáticas declaradas em algebra.h
// ============================================================

#include "algebra.h"

// =================== CONSTRUTORES DE TOKEN ==================
// Cada método estático cria um Token de um tipo específico

Token Token::numero(double v) {
    Token t;                    // Cria token com valores padrão
    t.tipo = NUMERO;            // Define o tipo como NUMERO
    t.valor = v;                // Armazena o valor numérico recebido
    return t;
}

Token Token::constante(double v) {
    Token t = numero(v);        // Vale como número...
    t.nomeada = true;           // ...mas permite multiplicação implícita: 2pi → 2*pi
    return t;
}

Token Token::invalido(const string& trecho) {
    Token t;
    t.tipo = INVALIDO;          // Marca o trecho como não reconhecido
    t.funcao = trecho;          // Guarda o texto para a mensagem de erro
    return t;
}

Token Token::op(char c) {
    Token t;
    t.tipo = OPERADOR;          // Define o tipo como OPERADOR
    t.operador = c;             // Armazena o caractere do operador (+, -, *, /, ^)
    return t;
}

Token Token::func(const string& nome) {
    Token t;
    t.tipo = FUNCAO;            // Define o tipo como FUNCAO
    t.funcao = nome;            // Armazena o nome da função (sin, cos, ln, etc.)
    return t;
}

Token Token::var(char c) {
    Token t;
    t.tipo = VARIAVEL;          // Define o tipo como VARIAVEL
    t.variavel = c;             // Armazena o caractere da variável ('x' ou 'y')
    return t;
}

Token Token::abreParen() {
    Token t;
    t.tipo = ABRE_PAREN;        // Define o tipo como parêntese de abertura
    return t;
}

Token Token::fechaParen() {
    Token t;
    t.tipo = FECHA_PAREN;       // Define o tipo como parêntese de fechamento
    return t;
}

// ====================== TOKENIZADOR =========================
// Transforma a string da expressão em um vetor de Tokens

vector<Token> tokenizar(const string& expressao) {
    vector<Token> tokens;        // Vetor que acumulará os tokens gerados

    // Converte a expressão para letras minúsculas com um laço simples
    string expr = expressao;
    for (size_t i = 0; i < expr.size(); i++) {
        expr[i] = tolower((unsigned char)expr[i]); // Converte cada caractere
    }

    size_t i = 0;                // Índice de leitura da string

    while (i < expr.size()) {    // Percorre cada caractere da expressão
        char c = expr[i];        // Caractere atual

        // Pula espaços em branco
        if (isspace((unsigned char)c)) {
            i++;
            continue;
        }

        // ---- Sinais unários ----
        // '+' e '-' são unários no início, após '(' ou após outro operador
        if (c == '-' || c == '+') {
            bool unario = tokens.empty() ||                       // Início da expressão
                          tokens.back().tipo == Token::OPERADOR || // Após operador (+, -, *, /, ^)
                          tokens.back().tipo == Token::ABRE_PAREN; // Após '('

            if (unario) {
                // O '-' unário é um operador próprio: assim -2^2 = -(2^2) e
                // e^-x = e^(-x). O '+' unário não muda o valor e é ignorado.
                if (c == '-') tokens.push_back(Token::op(OP_NEGACAO));
                i++;
                continue;
            }
        }

        // ---- Números (inteiros e decimais) ----
        if (isdigit((unsigned char)c) || c == '.') {
            string num;          // Acumula os dígitos
            while (i < expr.size() && (isdigit((unsigned char)expr[i]) || expr[i] == '.')) {
                num += expr[i];  // Concatena dígito ou ponto decimal
                i++;
            }
            double valorNum = 0.0;
            // No máximo um ponto decimal; "1.2.3" ou "." sozinho são recusados
            if (count(num.begin(), num.end(), '.') <= 1 &&
                (istringstream(num) >> valorNum)) {     // Converte string → double (sem exceção)
                tokens.push_back(Token::numero(valorNum));
            } else {
                tokens.push_back(Token::invalido(num));
            }
            continue;
        }

        // ---- Letras: variáveis, constantes ou funções ----
        if (isalpha((unsigned char)c)) {
            string palavra;      // Acumula os caracteres da palavra
            while (i < expr.size() && isalpha((unsigned char)expr[i])) {
                palavra += expr[i]; // Concatena cada letra
                i++;
            }

            // Verifica se é uma variável (x ou y)
            if (palavra == "x" || palavra == "y") {
                tokens.push_back(Token::var(palavra[0])); // Cria token de variável
            }
            // Verifica se é a constante π
            else if (palavra == "pi") {
                tokens.push_back(Token::constante(PI_VALOR)); // Substitui por valor numérico de π
            }
            // Verifica se é a constante e (Euler)
            else if (palavra == "e") {
                tokens.push_back(Token::constante(E_VALOR)); // Substitui por valor numérico de e
            }
            // Caso contrário, é um nome de função (sin, cos, ln, exp, etc.)
            else {
                tokens.push_back(Token::func(palavra)); // Cria token de função
            }
            continue;
        }

        // ---- Operadores aritméticos ----
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
            tokens.push_back(Token::op(c)); // Cria token de operador
            i++;
            continue;
        }

        // ---- Parênteses ----
        if (c == '(') {
            tokens.push_back(Token::abreParen()); // Cria token '('
            i++;
            continue;
        }
        if (c == ')') {
            tokens.push_back(Token::fechaParen()); // Cria token ')'
            i++;
            continue;
        }

        // Caractere desconhecido — registra para a validação recusar a expressão
        tokens.push_back(Token::invalido(string(1, c)));
        i++;
    }

    return tokens; // Retorna o vetor completo de tokens
}

// ============== VALIDAÇÃO SINTÁTICA ========================
// True se avaliarFuncao() sabe calcular a função com esse nome
static bool funcaoConhecida(const string& nome) {
    const char* nomes[] = {"sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh",
                           "sec", "csc", "cot", "ln", "log", "exp", "sqrt", "abs"};
    for (const char* n : nomes) {
        if (nome == n) return true;
    }
    return false;
}

// Monta a mensagem de erro para um trecho que o tokenizador não reconheceu
static string mensagemTrechoInvalido(const string& trecho) {
    if (trecho == ",") return "Erro de sintaxe: use ponto como separador decimal (ex: 2.5).";
    unsigned char c = trecho[0];
    if (isdigit(c) || c == '.') return "Erro de sintaxe: número mal formado '" + trecho + "'.";
    if (c < 128 && isprint(c)) return "Erro de sintaxe: caractere não reconhecido '" + trecho + "'.";
    // Acentos e símbolos como π ou ² chegam como bytes que o console não ecoa bem
    return "Erro de sintaxe: caractere não reconhecido (use letras sem acento, "
           "números, + - * / ^ e parênteses).";
}

// Valida os tokens (já com a multiplicação implícita inserida). Percorre a expressão
// alternando entre "espero um valor" e "espero um operador", o que garante que a
// pós-fixa gerada termina com exatamente um valor na pilha.
// Retorna true se a expressão é válida; caso contrário retorna false e descreve o
// problema em 'erro' (passagem por referência). Não usa exceções.
bool validarSintaxe(const vector<Token>& tokens, string& erro) {
    if (tokens.empty()) { erro = "Expressão vazia."; return false; }
    int parens = 0;
    bool esperaValor = true; // No início só pode vir um valor, função, '(' ou menos unário

    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& t = tokens[i];

        switch (t.tipo) {
            case Token::INVALIDO:
                erro = mensagemTrechoInvalido(t.funcao);
                return false;

            case Token::NUMERO:
            case Token::VARIAVEL:
                if (!esperaValor) {
                    erro = "Erro de sintaxe: falta um operador entre dois valores.";
                    return false;
                }
                esperaValor = false;
                break;

            case Token::FUNCAO:
                if (!funcaoConhecida(t.funcao)) {
                    erro = "Erro de sintaxe: nome desconhecido '" + t.funcao + "'.\n"
                           "  Funções: sin cos tan asin acos atan sinh cosh tanh sec csc cot "
                           "ln log exp sqrt abs\n"
                           "  Para multiplicar variáveis, use x*y.";
                    return false;
                }
                if (!esperaValor) {
                    erro = "Erro de sintaxe: falta um operador antes de '" + t.funcao + "'.";
                    return false;
                }
                if (i + 1 >= tokens.size() || tokens[i + 1].tipo != Token::ABRE_PAREN) {
                    erro = "Erro de sintaxe: a função '" + t.funcao + "' precisa de parênteses: " +
                           t.funcao + "(x).";
                    return false;
                }
                break;

            case Token::ABRE_PAREN:
                if (!esperaValor) {
                    erro = "Erro de sintaxe: falta um operador antes de '('.";
                    return false;
                }
                parens++;
                break;

            case Token::FECHA_PAREN:
                if (parens == 0) {
                    erro = "Erro de sintaxe: Parênteses desbalanceados (fechamento extra).";
                    return false;
                }
                if (esperaValor) {
                    erro = (tokens[i - 1].tipo == Token::ABRE_PAREN)
                               ? "Erro de sintaxe: Parênteses vazios."
                               : "Erro de sintaxe: Operador logo antes de fechar parêntese.";
                    return false;
                }
                parens--;
                break;

            case Token::OPERADOR:
                // O menos unário vem antes do valor, então continua esperando um
                if (t.operador == OP_NEGACAO) break;
                if (esperaValor) {
                    if (i == 0)
                        erro = "Erro de sintaxe: Operador no início da expressão.";
                    else if (tokens[i - 1].tipo == Token::ABRE_PAREN)
                        erro = "Erro de sintaxe: Operador logo após abrir parêntese.";
                    else
                        erro = "Erro de sintaxe: Operadores consecutivos.";
                    return false;
                }
                esperaValor = true;
                break;
        }
    }
    if (parens > 0) {
        erro = "Erro de sintaxe: Parênteses desbalanceados (abertura extra).";
        return false;
    }
    if (esperaValor) {
        erro = "Erro de sintaxe: Operador no final da expressão.";
        return false;
    }
    return true; // Tudo certo
}

// Verifica se a lista de tokens contém pelo menos uma ocorrência da variável 'var'.
// Usado para garantir que o usuário digitou uma função de fato (e não só uma
// constante como "e" ou "pi", que não há o que derivar/integrar).
bool contemVariavel(const vector<Token>& tokens, char var) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].tipo == Token::VARIAVEL && tokens[i].variavel == var) {
            return true;
        }
    }
    return false;
}

// ============== MULTIPLICAÇÃO IMPLÍCITA =====================
// Insere '*' entre tokens adjacentes que indicam multiplicação implícita
// Exemplo: "3x" → "3 * x", "2sin(x)" → "2 * sin(x)"

vector<Token> inserirMultImplicita(const vector<Token>& tokens) {
    vector<Token> resultado; // Vetor que receberá os tokens com '*' inserido

    for (size_t i = 0; i < tokens.size(); i++) {
        resultado.push_back(tokens[i]); // Copia o token atual para o resultado

        // Verifica se existe um próximo token
        if (i + 1 < tokens.size()) {
            Token::Tipo atual = tokens[i].tipo;     // Tipo do token atual
            Token::Tipo proximo = tokens[i + 1].tipo; // Tipo do próximo token
            bool inserir = false;                     // Flag para decidir se insere '*'

            // NUMERO seguido de VARIAVEL, FUNCAO, '(' ou constante (2pi) → multiplicação implícita
            // Dois números literais seguidos ("2 3") não entram: a validação recusa
            if (atual == Token::NUMERO &&
                (proximo == Token::VARIAVEL || proximo == Token::FUNCAO || proximo == Token::ABRE_PAREN ||
                 (proximo == Token::NUMERO && tokens[i + 1].nomeada))) {
                inserir = true;
            }
            // VARIAVEL seguido de VARIAVEL, FUNCAO, '(' ou NUMERO → multiplicação implícita
            if (atual == Token::VARIAVEL &&
                (proximo == Token::VARIAVEL || proximo == Token::FUNCAO ||
                 proximo == Token::ABRE_PAREN || proximo == Token::NUMERO)) {
                inserir = true;
            }
            // ')' seguido de NUMERO, VARIAVEL, FUNCAO ou '(' → multiplicação implícita
            if (atual == Token::FECHA_PAREN &&
                (proximo == Token::NUMERO || proximo == Token::VARIAVEL ||
                 proximo == Token::FUNCAO || proximo == Token::ABRE_PAREN)) {
                inserir = true;
            }

            if (inserir) {
                resultado.push_back(Token::op('*')); // Insere token de multiplicação
            }
        }
    }

    return resultado; // Retorna os tokens com multiplicações implícitas resolvidas
}

// ================ CONVERSÃO PÓS-FIXA =======================
// Algoritmo Shunting-yard de Dijkstra
// Converte expressão em notação infixa para pós-fixa (Reverse Polish Notation)

int precedencia(char op) {
    // Retorna a precedência numérica de cada operador
    if (op == '+' || op == '-') return 1;  // Adição e subtração: menor precedência
    if (op == '*' || op == '/') return 2;  // Multiplicação e divisão: precedência média
    if (op == OP_NEGACAO) return 3;        // Menos unário: acima de * e /, abaixo de ^
    if (op == '^') return 4;               // Potenciação: maior precedência
    return 0;                               // Operador desconhecido
}

bool ehAssocEsquerda(char op) {
    // Potenciação é associativa à direita (2^3^2 = 2^(3^2)), os demais à esquerda
    return op != '^';
}

vector<Token> paraPosfixo(const vector<Token>& tokens) {
    vector<Token> saida;       // Fila de saída em notação pós-fixa
    stack<Token> pilhaOps;     // Pilha de operadores e funções

    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& tok = tokens[i]; // Referência ao token atual

        switch (tok.tipo) {
            case Token::NUMERO:
            case Token::VARIAVEL:
                saida.push_back(tok); // Números e variáveis vão direto para a saída
                break;

            case Token::FUNCAO:
                pilhaOps.push(tok);   // Funções são empilhadas para posterior avaliação
                break;

            case Token::OPERADOR: {
                // O menos unário é prefixo: seu operando ainda não saiu, nada é desempilhado
                if (tok.operador == OP_NEGACAO) {
                    pilhaOps.push(tok);
                    break;
                }
                // Desempilha operadores de maior ou igual precedência (se assoc. esquerda)
                while (!pilhaOps.empty() &&
                       pilhaOps.top().tipo == Token::OPERADOR &&
                       ((ehAssocEsquerda(tok.operador) &&
                         precedencia(tok.operador) <= precedencia(pilhaOps.top().operador)) ||
                        (!ehAssocEsquerda(tok.operador) &&
                         precedencia(tok.operador) < precedencia(pilhaOps.top().operador)))) {
                    saida.push_back(pilhaOps.top()); // Move operador do topo para a saída
                    pilhaOps.pop();                   // Remove o operador da pilha
                }
                pilhaOps.push(tok); // Empilha o operador atual
                break;
            }

            case Token::ABRE_PAREN:
                pilhaOps.push(tok); // '(' é empilhado como marcador
                break;

            case Token::FECHA_PAREN:
                // Desempilha até encontrar o '(' correspondente
                while (!pilhaOps.empty() && pilhaOps.top().tipo != Token::ABRE_PAREN) {
                    saida.push_back(pilhaOps.top()); // Move operadores para a saída
                    pilhaOps.pop();
                }
                if (!pilhaOps.empty()) {
                    pilhaOps.pop(); // Remove o '(' da pilha (não vai para a saída)
                }
                // Se o topo agora é uma função, desempilha-a também
                if (!pilhaOps.empty() && pilhaOps.top().tipo == Token::FUNCAO) {
                    saida.push_back(pilhaOps.top()); // A função vai para a saída
                    pilhaOps.pop();
                }
                break;

            case Token::INVALIDO:
                break; // Nunca chega aqui: validarSintaxe recusa a expressão antes
        }
    }

    // Desempilha todos os operadores restantes para a saída
    while (!pilhaOps.empty()) {
        saida.push_back(pilhaOps.top());
        pilhaOps.pop();
    }

    return saida; // Retorna a expressão em notação pós-fixa
}

// ==================== AVALIADOR =============================
// Avalia uma expressão em notação pós-fixa para valores específicos de x e y

double avaliarFuncao(const string& nome, double argumento) {
    // Mapeia o nome da função para a chamada correspondente em <cmath>
    if (nome == "sin")  return sin(argumento);         // Seno
    if (nome == "cos")  return cos(argumento);         // Cosseno
    if (nome == "tan")  return tan(argumento);         // Tangente
    if (nome == "asin") return asin(argumento);        // Arco seno
    if (nome == "acos") return acos(argumento);        // Arco cosseno
    if (nome == "atan") return atan(argumento);        // Arco tangente
    if (nome == "ln")   return log(argumento);         // Logaritmo natural (base e)
    if (nome == "log")  return log10(argumento);       // Logaritmo decimal (base 10)
    if (nome == "exp")  return exp(argumento);         // Exponencial e^x
    if (nome == "sqrt") return sqrt(argumento);        // Raiz quadrada
    if (nome == "abs")  return abs(argumento);         // Valor absoluto
    if (nome == "sec")  return 1.0 / cos(argumento);  // Secante = 1/cos
    if (nome == "csc")  return 1.0 / sin(argumento);  // Cossecante = 1/sin
    if (nome == "cot")  return cos(argumento) / sin(argumento); // Cotangente = cos/sin
    if (nome == "sinh") return sinh(argumento);        // Seno hiperbólico
    if (nome == "cosh") return cosh(argumento);        // Cosseno hiperbólico
    if (nome == "tanh") return tanh(argumento);        // Tangente hiperbólica
    return NAN; // Função desconhecida retorna Not-a-Number
}

double avaliar(const vector<Token>& posfixo, double x, double y) {
    stack<double> pilha; // Pilha para avaliação da expressão pós-fixa

    // Percorre cada token da expressão pós-fixa usando iterador
    vector<Token>::const_iterator it;
    for (it = posfixo.begin(); it != posfixo.end(); ++it) {
        const Token& tok = *it; // Dereferencia o iterador para acessar o token

        switch (tok.tipo) {
            case Token::NUMERO:
                pilha.push(tok.valor); // Empilha o valor numérico
                break;

            case Token::VARIAVEL:
                // Empilha o valor da variável correspondente
                if (tok.variavel == 'x') pilha.push(x);
                else if (tok.variavel == 'y') pilha.push(y);
                break;

            case Token::OPERADOR: {
                if (tok.operador == OP_NEGACAO) {  // Menos unário: usa um só operando
                    if (pilha.empty()) return NAN;
                    pilha.top() = -pilha.top();
                    break;
                }
                if (pilha.size() < 2) return NAN; // Operador binário precisa de 2 operandos
                double b = pilha.top(); pilha.pop(); // Segundo operando (topo da pilha)
                double a = pilha.top(); pilha.pop(); // Primeiro operando

                // Aplica o operador aos dois operandos
                switch (tok.operador) {
                    case '+': pilha.push(a + b); break;  // Soma
                    case '-': pilha.push(a - b); break;  // Subtração
                    case '*': pilha.push(a * b); break;  // Multiplicação
                    case '/':
                        // Verifica divisão por zero usando operador relacional !=
                        pilha.push(b != 0.0 ? a / b : NAN);
                        break;
                    case '^': pilha.push(pow(a, b)); break; // Potenciação
                }
                break;
            }

            case Token::FUNCAO: {
                if (pilha.empty()) return NAN; // Função precisa de 1 argumento
                double arg = pilha.top(); pilha.pop(); // Retira o argumento da pilha
                pilha.push(avaliarFuncao(tok.funcao, arg)); // Avalia e empilha o resultado
                break;
            }

            default:
                break; // Ignora tokens de parênteses (não devem aparecer na pós-fixa)
        }
    }

    // O resultado final é o único valor restante na pilha
    return pilha.empty() ? NAN : pilha.top();
}

// ====================== CÁLCULO 1 ===========================

// ---- Limite pela Esquerda ----
// Aproxima lim(x→a⁻) f(x) avaliando f em pontos cada vez mais próximos de 'a' pela esquerda
double limiteEsquerda(const vector<Token>& f, double a) {
    const int N = 6;          // Número de aproximações a testar
    double valores[N];         // Array para armazenar as aproximações sucessivas
    double h = 0.1;            // Passo inicial de aproximação

    // Avalia f(a - h) para valores de h decrescentes: 0.1, 0.01, 0.001, ...
    for (int i = 0; i < N; i++) {
        valores[i] = avaliar(f, a - h); // Calcula f no ponto a - h
        h /= 10.0;                      // Reduz h por um fator de 10
    }

    // Verifica se a sequência está divergindo (|valores| crescendo rapidamente)
    bool divergindo = true;
    for (int i = 1; i < N; i++) {
        // Se algum valor é NaN ou não cresce 5x a cada passo, não está divergindo
        if (isnan(valores[i]) || isnan(valores[i - 1])) { divergindo = false; break; }
        if (abs(valores[i]) <= abs(valores[i - 1]) * 5.0) { divergindo = false; break; }
    }
    if (divergindo && abs(valores[N - 1]) > 1e4) {
        return (valores[N - 1] > 0) ? INFINITY : -INFINITY;
    }
    // Divergência lenta (ex: ln(x)→-∞): todos finitos, mesmo sinal, magnitude estritamente crescente
    if (!divergindo) {
        bool todoFinito = true;
        for (int i = 0; i < N; i++) {
            if (isnan(valores[i]) || isinf(valores[i])) { todoFinito = false; break; }
        }
        if (todoFinito) {
            bool monotonoDiv = true;
            double sinal0 = (valores[0] >= 0) ? 1.0 : -1.0;
            for (int i = 1; i < N; i++) {
                double sinalI = (valores[i] >= 0) ? 1.0 : -1.0;
                if (sinalI != sinal0 || abs(valores[i]) <= abs(valores[i - 1])) {
                    monotonoDiv = false; break;
                }
            }
            if (monotonoDiv && abs(valores[0]) > 1e-10 &&
                abs(valores[N - 1]) / abs(valores[0]) > 2.0 && abs(valores[N - 1]) > 5.0) {
                return (valores[N - 1] > 0) ? INFINITY : -INFINITY;
            }
        }
    }

    // Retorna a melhor aproximação finita (menor h)
    for (int i = N - 1; i >= 0; i--) {
        if (!isnan(valores[i]) && !isinf(valores[i])) {
            return valores[i]; // Primeiro valor válido de trás para frente
        }
    }

    // Se todos foram infinitos, retorna o sinal do infinito
    if (isinf(valores[0])) return valores[0];
    return NAN; // Nenhum valor válido encontrado
}

// ---- Limite pela Direita ----
// Análogo ao limite pela esquerda, mas aproxima por valores maiores que 'a'
double limiteDireita(const vector<Token>& f, double a) {
    const int N = 6;
    double valores[N];
    double h = 0.1;

    for (int i = 0; i < N; i++) {
        valores[i] = avaliar(f, a + h); // Avalia f no ponto a + h
        h /= 10.0;
    }

    bool divergindo = true;
    for (int i = 1; i < N; i++) {
        if (isnan(valores[i]) || isnan(valores[i - 1])) { divergindo = false; break; }
        if (abs(valores[i]) <= abs(valores[i - 1]) * 5.0) { divergindo = false; break; }
    }
    if (divergindo && abs(valores[N - 1]) > 1e4) {
        return (valores[N - 1] > 0) ? INFINITY : -INFINITY;
    }
    // Divergência lenta (ex: ln(x)→-∞): todos finitos, mesmo sinal, magnitude estritamente crescente
    if (!divergindo) {
        bool todoFinito = true;
        for (int i = 0; i < N; i++) {
            if (isnan(valores[i]) || isinf(valores[i])) { todoFinito = false; break; }
        }
        if (todoFinito) {
            bool monotonoDiv = true;
            double sinal0 = (valores[0] >= 0) ? 1.0 : -1.0;
            for (int i = 1; i < N; i++) {
                double sinalI = (valores[i] >= 0) ? 1.0 : -1.0;
                if (sinalI != sinal0 || abs(valores[i]) <= abs(valores[i - 1])) {
                    monotonoDiv = false; break;
                }
            }
            if (monotonoDiv && abs(valores[0]) > 1e-10 &&
                abs(valores[N - 1]) / abs(valores[0]) > 2.0 && abs(valores[N - 1]) > 5.0) {
                return (valores[N - 1] > 0) ? INFINITY : -INFINITY;
            }
        }
    }

    for (int i = N - 1; i >= 0; i--) {
        if (!isnan(valores[i]) && !isinf(valores[i])) {
            return valores[i];
        }
    }

    if (isinf(valores[0])) return valores[0];
    return NAN;
}

// ---- Limite Bilateral ----
// Verifica se os limites laterais coincidem; se sim, retorna o limite
double limite(const vector<Token>& f, double a) {
    double limEsq = limiteEsquerda(f, a); // Calcula limite pela esquerda
    double limDir = limiteDireita(f, a);   // Calcula limite pela direita

    // Se algum dos dois é NaN, o limite bilateral não existe
    if (isnan(limEsq) || isnan(limDir)) return NAN;

    // Se ambos são infinitos, verifica se têm o mesmo sinal
    if (isinf(limEsq) && isinf(limDir)) {
        // Usa operador lógico AND (&&) para verificar concordância de sinais
        if ((limEsq > 0 && limDir > 0) || (limEsq < 0 && limDir < 0)) {
            return limEsq; // Ambos +∞ ou ambos -∞
        }
        return NAN; // Sinais opostos: limite não existe
    }

    // Calcula tolerância relativa para comparar os limites laterais
    double tolerancia = max(1e-4, 1e-4 * max(abs(limEsq), abs(limDir)));

    // Se a diferença entre os limites laterais é menor que a tolerância
    if (abs(limEsq - limDir) < tolerancia) {
        return (limEsq + limDir) / 2.0; // Retorna a média como valor do limite
    }

    return NAN; // Limites laterais divergem: limite não existe
}

// ---- Continuidade ----
// Verifica as 3 condições de continuidade em x = a:
// 1) f(a) está definida   2) lim f(x) existe   3) lim f(x) = f(a)
bool verificarContinuidade(const vector<Token>& f, double a) {
    double fa = avaliar(f, a);  // Avalia f(a) diretamente
    double lim = limite(f, a);   // Calcula o limite bilateral

    bool fDefinida = !isnan(fa) && !isinf(fa);  // Condição 1: f(a) existe e é finita
    bool limExiste = !isnan(lim) && !isinf(lim); // Condição 2: o limite existe e é finito

    // Exibe diagnóstico detalhado das condições
    cout << "\n  Análise de continuidade em x = " << a << ":" << endl;

    cout << "  1) f(" << a << ") = ";
    if (fDefinida) cout << fa << " (definida)" << endl;
    else cout << "indefinida" << endl;

    cout << "  2) lim f(x) quando x→" << a << " = ";
    if (limExiste) cout << lim << " (existe)" << endl;
    else cout << "não existe" << endl;

    // Usa operador XOR (^) para detectar quando exatamente uma condição falha
    if (fDefinida ^ limExiste) {
        cout << "  3) Não é possível comparar (uma das condições anteriores falhou)" << endl;
        cout << "\n  Resultado: f é DESCONTÍNUA em x = " << a << endl;
        return false;
    }

    // Ambas existem: verifica se são iguais
    if (fDefinida && limExiste) {
        bool iguais = abs(fa - lim) < max(1e-6, 1e-6 * abs(fa)); // Comparação com tolerância
        cout << "  3) lim f(x) " << (iguais ? "=" : "≠") << " f(" << a << ")" << endl;

        if (iguais) {
            cout << "\n  Resultado: f é CONTÍNUA em x = " << a << endl;
            return true;
        } else {
            cout << "\n  Resultado: f é DESCONTÍNUA em x = " << a
                 << " (descontinuidade removível)" << endl;
            return false;
        }
    }

    // Nenhuma das condições é satisfeita
    cout << "\n  Resultado: f é DESCONTÍNUA em x = " << a << endl;
    return false;
}

// ---- Primeira Derivada ----
// Usa a fórmula de diferença central: f'(x) ≈ [f(x+h) - f(x-h)] / (2h)
double derivada(const vector<Token>& f, double x) {
    double h = 1e-8; // Incremento infinitesimal para aproximação
    double fxph = avaliar(f, x + h); // f(x + h)
    double fxmh = avaliar(f, x - h); // f(x - h)
    return (fxph - fxmh) / (2.0 * h); // Fórmula da diferença central
}

// ---- Segunda Derivada ----
// Usa a fórmula: f''(x) ≈ [f(x+h) - 2f(x) + f(x-h)] / h²
double derivadaSegunda(const vector<Token>& f, double x) {
    double h = 1e-5; // h maior que na primeira derivada para estabilidade numérica
    double fxph = avaliar(f, x + h);  // f(x + h)
    double fx   = avaliar(f, x);      // f(x)
    double fxmh = avaliar(f, x - h);  // f(x - h)
    return (fxph - 2.0 * fx + fxmh) / (h * h); // Fórmula da segunda diferença central
}

// ---- Derivada Implícita ----
// Para F(x,y) = 0, calcula dy/dx = -(∂F/∂x) / (∂F/∂y) no ponto (x₀, y₀)
double derivadaImplicita(const vector<Token>& F, double x0, double y0) {
    double h = 1e-8; // Incremento para derivadas parciais

    // ∂F/∂x ≈ [F(x₀+h, y₀) - F(x₀-h, y₀)] / (2h) — derivada parcial em relação a x
    double dFdx = (avaliar(F, x0 + h, y0) - avaliar(F, x0 - h, y0)) / (2.0 * h);

    // ∂F/∂y ≈ [F(x₀, y₀+h) - F(x₀, y₀-h)] / (2h) — derivada parcial em relação a y
    double dFdy = (avaliar(F, x0, y0 + h) - avaliar(F, x0, y0 - h)) / (2.0 * h);

    // Verifica se ∂F/∂y ≈ 0 (denominador da fração)
    if (abs(dFdy) < 1e-15) return NAN; // Derivada indefinida se ∂F/∂y = 0

    return -(dFdx / dFdy); // Aplica a fórmula dy/dx = -(∂F/∂x) / (∂F/∂y)
}

// True se v é um número comum (nem NAN nem infinito)
static bool finito(double v) {
    return !isnan(v) && !isinf(v);
}

// Ponto a + i·h de uma malha. Um resíduo de arredondamento como 1e-17 vira 0 exato:
// em expressões como (x - sin(x))/x³ ele daria um valor completamente errado.
static double pontoDaMalha(double a, double h, int i) {
    double x = a + i * h;
    return (fabs(x) < 1e-9 * fabs(h)) ? 0.0 : x;
}

// f(x); se f não está definida exatamente em x (ex.: sin(x)/x em 0),
// usa a média dos dois vizinhos
static double valorNoPonto(const vector<Token>& f, double x) {
    double v = avaliar(f, x);
    if (finito(v)) return v;
    double d = max(1e-5, 1e-10 * fabs(x));
    return (avaliar(f, x - d) + avaliar(f, x + d)) / 2.0;
}

// ---- Integral Definida (Regra de Simpson 1/3) ----
// Calcula ∫[a,b] f(x)dx usando a regra de Simpson composta.
// Não detecta divergência: para isso use integrarNumericamente().
double integralDefinida(const vector<Token>& f, double a, double b, int n) {
    // A regra de Simpson exige n par
    if (n % 2 != 0) n++; // Incrementa n se for ímpar

    double h = (b - a) / n;                  // Largura de cada subdivisão
    double soma = valorNoPonto(f, a) + valorNoPonto(f, b); // Soma inicia com f(a) + f(b)

    // Percorre os pontos intermediários com pesos alternados (4 e 2)
    for (int i = 1; i < n; i++) {
        double xi = pontoDaMalha(a, h, i);    // Ponto x_i na subdivisão
        double fx = valorNoPonto(f, xi);      // Avalia f(x_i)

        // Índices ímpares recebem peso 4, pares recebem peso 2
        if (i % 2 != 0) {
            soma += 4.0 * fx;                // Peso 4 para pontos ímpares
        } else {
            soma += 2.0 * fx;                // Peso 2 para pontos pares
        }
    }

    return (h / 3.0) * soma; // Multiplica pelo fator h/3 da regra de Simpson
}

// ---- Singularidades ----
// Maior |f| em 8 pontos a uma distância próxima de h do ponto s, do lado
// indicado (-1 ou +1). Retorna NAN se f não existe em nenhum deles.
static double picoPerto(const vector<Token>& f, double s, int lado, double h) {
    double pico = NAN;
    for (int k = 0; k < 8; k++) {
        double v = fabs(avaliar(f, s + lado * h * (1.0 + k / 8.0)));
        if (!isnan(v) && !(v <= pico)) pico = v;
    }
    return pico;
}

// |f(x)|, tratando um ponto sem valor como infinitamente grande
static double moduloOuInfinito(const vector<Token>& f, double x) {
    double v = fabs(avaliar(f, x));
    return isnan(v) ? INFINITY : v;
}

// Busca, por seção áurea, o ponto de [lo, hi] em que |f| é máximo
static double acharPico(const vector<Token>& f, double lo, double hi) {
    const double R = 0.6180339887498949;
    double c = hi - R * (hi - lo), d = lo + R * (hi - lo);
    double fc = moduloOuInfinito(f, c), fd = moduloOuInfinito(f, d);
    for (int i = 0; i < 80; i++) {
        if (fc >= fd) {
            hi = d; d = c; fd = fc;
            c = hi - R * (hi - lo); fc = moduloOuInfinito(f, c);
        } else {
            lo = c; c = d; fc = fd;
            d = lo + R * (hi - lo); fd = moduloOuInfinito(f, d);
        }
    }
    return (lo + hi) / 2.0;
}

// True se f não tem valor em s ou se |f| continua crescendo ao se aproximar de s
static bool ehSingular(const vector<Token>& f, double s, double a, double b) {
    if (!finito(avaliar(f, s))) return true;
    double escala = max(fabs(s), min(1.0, b - a));
    for (int lado = -1; lado <= 1; lado += 2) {
        double vizinho = s + lado * 1e-8 * escala;
        if (vizinho < a || vizinho > b) continue; // lado fora do intervalo
        if (picoPerto(f, s, lado, 1e-12 * escala) >
            1.2 * picoPerto(f, s, lado, 1e-8 * escala)) return true;
    }
    return false;
}

// Procura em [a, b] os pontos em que f não é limitada ou não está definida.
// Retorna false (e o local em 'problema') se f não existir, ou estourar o double,
// em um trecho inteiro.
static bool acharSingularidades(const vector<Token>& f, double a, double b,
                                vector<double>& pontos, double& problema) {
    const int M = 2000;
    double passo = (b - a) / M;
    vector<double> modulo(M + 1);
    for (int i = 0; i <= M; i++)
        modulo[i] = fabs(avaliar(f, i == M ? b : pontoDaMalha(a, passo, i)));

    for (int i = 0; i <= M; i++) {
        double x = (i == M) ? b : pontoDaMalha(a, passo, i);

        if (!finito(modulo[i])) {
            // Um ponto isolado pode ser integrável; um trecho sem valor não
            double d = 1e-9 * max(1.0, fabs(x));
            bool trecho = (i > 0 && isnan(avaliar(f, x - d))) ||
                          (i < M && isnan(avaliar(f, x + d))) ||
                          (i > 0 && i < M && !finito(modulo[i - 1]) && !finito(modulo[i + 1]));
            if (trecho) { problema = x; return false; }
            pontos.push_back(x);
            continue;
        }

        // Perto de um polo, |f| tem um máximo local entre os pontos amostrados
        bool maximo = (i == 0 || modulo[i] > modulo[i - 1]) &&
                      (i == M || modulo[i] >= modulo[i + 1]);
        if (!maximo) continue;
        double s = acharPico(f, max(a, x - passo), min(b, x + passo));
        if (fabs(s - a) < 1e-9 * max(1.0, fabs(a))) s = a;
        if (fabs(s - b) < 1e-9 * max(1.0, fabs(b))) s = b;
        if (ehSingular(f, s, a, b)) pontos.push_back(s);
    }
    return true;
}

// Estima p em |f(s ± h)| ~ C/h^p de um lado da singularidade. A integral existe
// se p < 1; com fator logarítmico perto de p = 1 não dá para decidir.
static StatusIntegral classificarLado(const vector<Token>& f, double s, int lado,
                                      double escala, double& p) {
    double m4 = picoPerto(f, s, lado, 1e-4 * escala);
    double m6 = picoPerto(f, s, lado, 1e-6 * escala);
    double m8 = picoPerto(f, s, lado, 1e-8 * escala);
    p = 0.0;
    if (isinf(m4) || isinf(m6) || isinf(m8)) return INTEGRAL_DIVERGE;
    if (isnan(m4) || isnan(m6) || isnan(m8)) return INTEGRAL_INDEFINIDA;
    if (m6 == 0.0 || m8 == 0.0) return INTEGRAL_OK;

    double p1 = (m4 == 0.0) ? 0.0 : log10(m6 / m4) / 2.0;
    double p2 = log10(m8 / m6) / 2.0;
    if (p2 >= 0.999) return INTEGRAL_DIVERGE;
    if (p2 > 0.7 && fabs(p2 - p1) > 0.005) return INTEGRAL_INCERTA;
    p = max(p2, 0.0);
    return INTEGRAL_OK;
}

// Integra entre a singularidade s e o ponto 'ate' com a mudança de variável
// x = s ± e^v, que concentra os pontos de Simpson perto da singularidade.
// O trecho minúsculo colado em s é estimado supondo f ~ C/h^p.
static double integralDesdeSingularidade(const vector<Token>& f, double s, double ate,
                                         double p, int n) {
    double lado = (ate > s) ? 1.0 : -1.0;
    double largura = fabs(ate - s);
    double delta = max(1e-10 * largura, 1e-13 * max(1.0, fabs(s)));
    double cauda = avaliar(f, s + lado * delta) * delta / (1.0 - p);

    if (n % 2 != 0) n++;
    double v0 = log(delta), v1 = log(largura);
    double h = (v1 - v0) / n;
    double soma = 0.0;
    for (int i = 0; i <= n; i++) {
        double t = exp(v0 + i * h);
        double peso = (i == 0 || i == n) ? 1.0 : (i % 2 != 0 ? 4.0 : 2.0);
        soma += peso * valorNoPonto(f, s + lado * t) * t; // dx = e^v dv
    }
    return cauda + (h / 3.0) * soma;
}

// Um extremo de subintervalo e o comportamento de f ao redor dele
struct Fronteira {
    double x;
    bool singular;
    double pEsquerda, pDireita; // expoentes p de cada lado
};

// Integra de uma fronteira à seguinte, tratando as pontas singulares
static double integrarPedaco(const vector<Token>& f, const Fronteira& ini,
                             const Fronteira& fim, int n) {
    if (ini.singular && fim.singular) {
        double m = (ini.x + fim.x) / 2.0;
        return integralDesdeSingularidade(f, ini.x, m, ini.pDireita, n / 5) +
               integralDesdeSingularidade(f, fim.x, m, fim.pEsquerda, n / 5);
    }
    if (ini.singular) return integralDesdeSingularidade(f, ini.x, fim.x, ini.pDireita, n / 5);
    if (fim.singular) return integralDesdeSingularidade(f, fim.x, ini.x, fim.pEsquerda, n / 5);
    return integralDefinida(f, ini.x, fim.x, n);
}

// ---- Integral Numérica com Verificação ----
// Calcula ∫[a,b] f(x)dx dividindo o intervalo nas singularidades. Informa quando
// a integral diverge ou não está definida, em vez de devolver um número sem sentido.
ResultadoIntegral integrarNumericamente(const vector<Token>& f, double a, double b, int n) {
    ResultadoIntegral r;
    r.status = INTEGRAL_OK;
    r.valor = 0.0;
    r.ponto = NAN;
    if (!finito(a) || !finito(b)) {
        r.status = INTEGRAL_INDEFINIDA;
        r.valor = NAN;
        return r;
    }
    if (a == b) return r;

    double sinal = 1.0;
    if (a > b) { swap(a, b); sinal = -1.0; } // ∫[a,b] = -∫[b,a]

    vector<double> pontos;
    if (!acharSingularidades(f, a, b, pontos, r.ponto)) {
        r.status = INTEGRAL_INDEFINIDA;
        r.valor = NAN;
        return r;
    }
    sort(pontos.begin(), pontos.end());

    // Monta as fronteiras: a, as singularidades (sem repetição) e b
    Fronteira inicio = {a, false, 0.0, 0.0};
    vector<Fronteira> fronteiras(1, inicio);
    for (size_t i = 0; i < pontos.size(); i++) {
        double s = pontos[i];
        if (s - fronteiras.back().x > 1e-9 * max(1.0, fabs(s))) {
            Fronteira nova = {s, true, 0.0, 0.0};
            fronteiras.push_back(nova);
        } else {
            fronteiras.back().singular = true;
        }
    }
    if (b - fronteiras.back().x > 1e-9 * max(1.0, fabs(b))) {
        Fronteira fim = {b, false, 0.0, 0.0};
        fronteiras.push_back(fim);
    }

    // Classifica cada lado de cada singularidade; divergência tem prioridade
    for (size_t i = 0; i < fronteiras.size(); i++) {
        Fronteira& s = fronteiras[i];
        if (!s.singular) continue;
        double escala = max(fabs(s.x), min(1.0, b - a));
        for (int lado = -1; lado <= 1; lado += 2) {
            if ((lado < 0 && i == 0) || (lado > 0 && i + 1 == fronteiras.size())) continue;
            double p;
            StatusIntegral st = classificarLado(f, s.x, lado, escala, p);
            if (lado < 0) s.pEsquerda = p; else s.pDireita = p;
            if (st != INTEGRAL_OK && (r.status == INTEGRAL_OK || st == INTEGRAL_DIVERGE)) {
                r.status = st;
                r.ponto = s.x;
            }
        }
    }
    if (r.status != INTEGRAL_OK) {
        r.valor = NAN;
        return r;
    }

    double total = 0.0, erro = 0.0;
    for (size_t i = 0; i + 1 < fronteiras.size(); i++) {
        double fino = integrarPedaco(f, fronteiras[i], fronteiras[i + 1], n);
        double grosso = integrarPedaco(f, fronteiras[i], fronteiras[i + 1], n / 2);
        total += fino;
        erro += fabs(fino - grosso); // diferença entre as malhas estima o erro
    }
    r.valor = sinal * total;
    if (!finito(total)) r.status = INTEGRAL_INDEFINIDA;
    else if (erro > 1e-6 * max(1.0, fabs(total))) r.status = INTEGRAL_IMPRECISA;
    return r;
}

// True se o resultado traz um valor utilizável
static bool temValor(const ResultadoIntegral& r) {
    return r.status == INTEGRAL_OK || r.status == INTEGRAL_IMPRECISA;
}

// ---- Integral Indefinida (Exibição em Tabela) ----
// Mostra uma tabela de F(x) = ∫ f(t)dt, de uma referência até x, para x em [xMin, xMax]
void mostrarIntegralIndefinida(const vector<Token>& f, double xMin, double xMax, int pontos,
                               const string& arquivoCSV) {
    double passo = (xMax - xMin) / (pontos - 1); // Distância entre cada ponto da tabela

    // Referência F(r) = 0: usa r = 0 se der para integrar a partir dele; senão
    // (ex.: 1/x) escolhe outro ponto
    const double candidatos[] = {0.0, 1.0, xMin, (xMin + xMax) / 2.0, xMax};
    double ref = 0.0;
    for (double c : candidatos) {
        if ((c != xMin && temValor(integrarNumericamente(f, c, xMin, 2000))) ||
            (c != xMax && temValor(integrarNumericamente(f, c, xMax, 2000)))) {
            ref = c;
            break;
        }
    }

    cout << "\n  Integral indefinida: F(x) = ∫f(x)dx + C" << endl;
    cout << "  (Valores calculados com referência F(" << ref << ") = 0)\n" << endl;
    cout << "  ┌─────────────────┬─────────────────┐" << endl;
    cout << "  │        x        │      F(x) + C   │" << endl;
    cout << "  ├─────────────────┼─────────────────┤" << endl;

    // Abre o arquivo CSV se um nome foi informado (manipulação de arquivo de texto)
    ofstream csv;
    if (!arquivoCSV.empty()) {
        csv.open(arquivoCSV);
        if (csv) csv << "x,F(x)\n"; // cabeçalho do CSV
    }

    // Calcula e exibe F(x) para cada ponto da tabela
    for (int i = 0; i < pontos; i++) {
        double x = xMin + i * passo;                       // Valor de x atual
        ResultadoIntegral r = integrarNumericamente(f, ref, x, 10000);

        // Texto da célula: o valor de F(x) ou o motivo de não existir
        ostringstream celula;
        if (temValor(r)) celula << fixed << setprecision(4) << r.valor;
        else if (r.status == INTEGRAL_DIVERGE) celula << "diverge";
        else if (r.status == INTEGRAL_INCERTA) celula << "incerta";
        else celula << "indefinida";

        cout << "  │ " << setw(15) << fixed << setprecision(4) << x
             << " │ " << setw(15) << celula.str() << " │" << endl;
        if (csv) csv << x << "," << celula.str() << "\n"; // grava a linha no CSV
    }

    cout << "  └─────────────────┴─────────────────┘" << endl;
    if (csv) {
        csv.close();
        cout << "\n  Tabela exportada para o arquivo: " << arquivoCSV << endl;
    }
    cout << "\n  Lembre-se de adicionar a constante de integração C!" << endl;
}

// ====================== CÁLCULO 2 ===========================

// ---- Aritmética de Séries (base dos coeficientes de Taylor) ----
// Uma Serie guarda os coeficientes r[k] = r^(k)(a) / k! de uma expressão em torno
// de a. Aplicando cada operação da pós-fixa sobre séries inteiras, os coeficientes
// saem com a precisão do double em qualquer ordem. Isso evita as diferenças finitas
// (instáveis em ordens altas) e a derivação repetida da árvore (que cresce
// exponencialmente em quocientes). Um coeficiente que não existe vira NAN ou infinito.
typedef vector<double> Serie;

// Série de um valor constante: [v, 0, 0, ...]
static Serie serieConstante(double v, size_t n) {
    Serie r(n, 0.0);
    r[0] = v;
    return r;
}

static bool serieEhConstante(const Serie& u) {
    for (size_t k = 1; k < u.size(); k++) {
        if (u[k] != 0.0) return false;
    }
    return true;
}

// u + sinal·v, termo a termo
static Serie somarSeries(const Serie& u, const Serie& v, double sinal) {
    Serie r(u.size());
    for (size_t k = 0; k < u.size(); k++) r[k] = u[k] + sinal * v[k];
    return r;
}

// Produto de Cauchy: r[k] = Σ u[j]·v[k-j]
static Serie multiplicarSeries(const Serie& u, const Serie& v) {
    Serie r(u.size(), 0.0);
    for (size_t k = 0; k < u.size(); k++) {
        for (size_t j = 0; j <= k; j++) r[k] += u[j] * v[k - j];
    }
    return r;
}

// r = u / v, isolando r[k] em u = r·v
static Serie dividirSeries(const Serie& u, const Serie& v) {
    Serie r(u.size(), NAN);
    if (v[0] == 0.0) return r; // divisão por zero no ponto a
    for (size_t k = 0; k < u.size(); k++) {
        double soma = u[k];
        for (size_t j = 1; j <= k; j++) soma -= v[j] * r[k - j];
        r[k] = soma / v[0];
    }
    return r;
}

// Série r com r(a) = valor e r' = du / den (usada por ln, atan, asin e acos)
static Serie integrarQuociente(double valor, const Serie& u, const Serie& den) {
    Serie du(u.size(), 0.0); // derivada de u: du[k] = (k+1)·u[k+1]
    for (size_t k = 0; k + 1 < u.size(); k++) du[k] = (k + 1) * u[k + 1];
    Serie d = dividirSeries(du, den);
    Serie r(u.size());
    r[0] = valor;
    for (size_t k = 1; k < u.size(); k++) r[k] = d[k - 1] / k;
    return r;
}

// e^u, pela relação r' = u'·r
static Serie exponencialSerie(const Serie& u) {
    Serie r(u.size(), 0.0);
    r[0] = exp(u[0]);
    for (size_t k = 1; k < u.size(); k++) {
        for (size_t j = 1; j <= k; j++) r[k] += j * u[j] * r[k - j];
        r[k] /= k;
    }
    return r;
}

// Calcula juntos seno e cosseno de u (ou senh e cosh, se hiperbolico = true),
// pelas relações s' = c·u' e c' = -s·u' (sem o sinal negativo nas hiperbólicas)
static void senoCossenoSerie(const Serie& u, bool hiperbolico, Serie& s, Serie& c) {
    s.assign(u.size(), 0.0);
    c.assign(u.size(), 0.0);
    s[0] = hiperbolico ? sinh(u[0]) : sin(u[0]);
    c[0] = hiperbolico ? cosh(u[0]) : cos(u[0]);
    for (size_t k = 1; k < u.size(); k++) {
        double somaS = 0.0, somaC = 0.0;
        for (size_t j = 1; j <= k; j++) {
            somaS += j * u[j] * c[k - j];
            somaC += j * u[j] * s[k - j];
        }
        s[k] = somaS / k;
        c[k] = (hiperbolico ? somaC : -somaC) / k;
    }
}

// u^v
static Serie potenciaSerie(const Serie& u, const Serie& v) {
    size_t n = u.size();

    // Expoente variável: u^v = e^(v·ln u), que só existe para u > 0
    if (!serieEhConstante(v)) {
        if (!(u[0] > 0)) return Serie(n, NAN);
        return exponencialSerie(multiplicarSeries(v, integrarQuociente(log(u[0]), u, u)));
    }

    double p = v[0];
    if (serieEhConstante(u)) return serieConstante(pow(u[0], p), n);

    // Expoente inteiro não negativo: multiplicações repetidas (vale mesmo se u(a) = 0)
    if (p >= 0 && p <= 1000 && p == floor(p)) {
        Serie r = serieConstante(1.0, n);
        Serie base = u;
        for (int e = (int)p; e > 0; e /= 2) { // exponenciação por quadrados
            if (e % 2 != 0) r = multiplicarSeries(r, base);
            base = multiplicarSeries(base, base);
        }
        return r;
    }

    // Demais expoentes, pela relação u·r' = p·r·u'
    Serie r(n, NAN);
    r[0] = pow(u[0], p);
    if (u[0] == 0.0) return r; // ex.: sqrt(x) não é derivável em 0
    for (size_t k = 1; k < n; k++) {
        double soma = 0.0;
        for (size_t j = 1; j <= k; j++) soma += (p * j - (k - j)) * u[j] * r[k - j];
        r[k] = soma / (k * u[0]);
    }
    return r;
}

// Aplica uma das funções reconhecidas pelo parser a uma série
static Serie funcaoSerie(const string& nome, const Serie& u) {
    size_t n = u.size();
    double u0 = u[0];

    // Argumento constante: o resultado também é constante
    if (serieEhConstante(u)) return serieConstante(avaliarFuncao(nome, u0), n);

    Serie um = serieConstante(1.0, n);
    if (nome == "exp")  return exponencialSerie(u);
    if (nome == "sqrt") return potenciaSerie(u, serieConstante(0.5, n));
    if (nome == "ln" || nome == "log") {
        if (!(u0 > 0)) return Serie(n, NAN);
        Serie r = integrarQuociente(log(u0), u, u);          // ln' = u'/u
        if (nome == "log") {                                  // log10(u) = ln(u)/ln(10)
            for (size_t k = 0; k < n; k++) r[k] /= log(10.0);
        }
        return r;
    }
    if (nome == "atan") {                                     // atan' = u'/(1+u²)
        return integrarQuociente(atan(u0), u, somarSeries(um, multiplicarSeries(u, u), 1.0));
    }
    if (nome == "asin" || nome == "acos") {                   // asin' = u'/√(1-u²) = -acos'
        Serie raiz = potenciaSerie(somarSeries(um, multiplicarSeries(u, u), -1.0),
                                   serieConstante(0.5, n));
        if (nome == "asin") return integrarQuociente(asin(u0), u, raiz);
        Serie menosRaiz = somarSeries(serieConstante(0.0, n), raiz, -1.0);
        return integrarQuociente(acos(u0), u, menosRaiz);
    }
    if (nome == "abs") {
        if (u0 > 0) return u;
        if (u0 < 0) return somarSeries(serieConstante(0.0, n), u, -1.0);
        Serie r(n, NAN);                                      // |u| em u = 0: sem derivada
        r[0] = 0.0;
        return r;
    }

    bool hiperbolico = (nome == "sinh" || nome == "cosh" || nome == "tanh");
    Serie s, c;
    senoCossenoSerie(u, hiperbolico, s, c);
    if (nome == "sin" || nome == "sinh") return s;
    if (nome == "cos" || nome == "cosh") return c;
    if (nome == "tan" || nome == "tanh") return dividirSeries(s, c);
    if (nome == "sec") return dividirSeries(um, c);
    if (nome == "csc") return dividirSeries(um, s);
    if (nome == "cot") return dividirSeries(c, s);
    return Serie(n, NAN);                                     // função desconhecida
}

// ---- Coeficientes de Taylor ----
// Calcula os coeficientes aₙ = f^(n)(a) / n! para n = 0, 1, ..., numTermos-1
// Percorre a pós-fixa como avaliar(), mas com uma pilha de séries
vector<double> calcularCoefsTaylor(const vector<Token>& f, double a, int numTermos) {
    if (numTermos <= 0) return vector<double>();
    size_t n = numTermos;
    stack<Serie> pilha;

    for (size_t i = 0; i < f.size(); i++) {
        const Token& tok = f[i];

        switch (tok.tipo) {
            case Token::NUMERO:
                pilha.push(serieConstante(tok.valor, n));
                break;

            case Token::VARIAVEL: {
                // x = a + 1·(x-a); y vale 0, como em avaliar() sem y
                Serie v = serieConstante(tok.variavel == 'x' ? a : 0.0, n);
                if (tok.variavel == 'x' && n > 1) v[1] = 1.0;
                pilha.push(v);
                break;
            }

            case Token::OPERADOR: {
                if (pilha.empty()) return Serie(n, NAN);
                Serie v = pilha.top(); pilha.pop();   // Segundo operando (ou o único)
                if (tok.operador == OP_NEGACAO) {
                    pilha.push(somarSeries(serieConstante(0.0, n), v, -1.0));
                    break;
                }
                if (pilha.empty()) return Serie(n, NAN);
                Serie u = pilha.top(); pilha.pop();   // Primeiro operando
                switch (tok.operador) {
                    case '+': pilha.push(somarSeries(u, v, 1.0)); break;
                    case '-': pilha.push(somarSeries(u, v, -1.0)); break;
                    case '*': pilha.push(multiplicarSeries(u, v)); break;
                    case '/': pilha.push(dividirSeries(u, v)); break;
                    case '^': pilha.push(potenciaSerie(u, v)); break;
                }
                break;
            }

            case Token::FUNCAO: {
                if (pilha.empty()) return Serie(n, NAN);
                Serie u = pilha.top(); pilha.pop();
                pilha.push(funcaoSerie(tok.funcao, u));
                break;
            }

            default:
                break; // Parênteses não aparecem na pós-fixa
        }
    }

    return pilha.empty() ? Serie(n, NAN) : pilha.top();
}

// ---- Integração por Série de Taylor ----
// Integra termo a termo em [lo, hi] a série de f em torno de c:
//   ∫ Σ aₖ(x-c)^k dx = Σ aₖ [(hi-c)^(k+1) - (lo-c)^(k+1)] / (k+1)
// Retorna false se a série não existe, não converge no trecho ou perde precisão.
static bool integrarSerieEm(const vector<Token>& f, double c, double lo, double hi,
                            Serie& coefs, double& valor, int& grau) {
    const int N = 80;      // termos calculados
    const int CAUDA = 20;  // os últimos precisam ser desprezíveis
    coefs = calcularCoefsTaylor(f, c, N);

    double u = lo - c, v = hi - c;
    double raio = max(fabs(u), fabs(v));
    double potU = u, potV = v, potRaio = raio;
    vector<double> tamanho(N); // maior valor que o termo k assume no trecho
    double maior = 0.0;
    valor = 0.0;
    for (int k = 0; k < N; k++) {
        if (!finito(coefs[k])) return false;
        valor += coefs[k] * (potV - potU) / (k + 1);
        tamanho[k] = fabs(coefs[k]) * potRaio / (k + 1);
        if (tamanho[k] > maior) maior = tamanho[k];
        potU *= u;
        potV *= v;
        potRaio *= raio;
    }
    if (!finito(valor)) return false;

    // Confere a série contra f em pontos do trecho: recusa funções não suaves
    // (ex.: abs) e coeficientes que "zeram" por limitação do double
    for (int i = 0; i <= 8; i++) {
        double x = pontoDaMalha(lo, (hi - lo) / 8.0, i);
        double real = avaliar(f, x);
        if (!finito(real)) continue; // singularidade removível
        double aprox = avaliarSerieTaylor(coefs, c, x);
        if (!(fabs(aprox - real) <= 1e-7 * max(1.0, fabs(real)))) return false;
    }

    double escala = max(1.0, fabs(valor));
    double tol = 1e-12 * escala;
    if (maior > 1e4 * escala) return false; // termos enormes se cancelando

    grau = 0;
    for (int k = 0; k < N; k++) {
        if (tamanho[k] > tol) {
            if (k >= N - CAUDA) return false; // ainda não convergiu
            grau = k + 1;                     // grau do termo na primitiva
        }
    }
    return true;
}

// Calcula ∫[a,b] f(x)dx por séries de Taylor. Tenta uma única série (centrada em 0,
// se estiver no intervalo, ou no ponto médio); se não convergir, divide [a, b] em
// partes menores, cada uma com a série centrada no seu meio.
IntegralTaylor integrarPorTaylor(const vector<Token>& f, double a, double b) {
    IntegralTaylor r;
    r.convergiu = false;
    r.valor = NAN;
    r.pedacos = 1;
    r.grau = 0;
    r.centro = (a + b) / 2.0;
    if (!finito(a) || !finito(b)) return r;

    // 1) Uma única série
    vector<double> centros;
    if (min(a, b) <= 0.0 && max(a, b) >= 0.0) centros.push_back(0.0);
    centros.push_back((a + b) / 2.0);
    for (size_t i = 0; i < centros.size(); i++) {
        if (integrarSerieEm(f, centros[i], a, b, r.coefs, r.valor, r.grau)) {
            r.convergiu = true;
            r.centro = centros[i];
            return r;
        }
    }

    // 2) Várias séries, dobrando o número de partes
    for (int partes = 2; partes <= 64; partes *= 2) {
        double largura = (b - a) / partes;
        double soma = 0.0;
        int maiorGrau = 0;
        bool todas = true;
        for (int i = 0; i < partes && todas; i++) {
            double lo = a + i * largura;
            double hi = (i == partes - 1) ? b : a + (i + 1) * largura;
            Serie coefs;
            double valor;
            int grau;
            todas = integrarSerieEm(f, (lo + hi) / 2.0, lo, hi, coefs, valor, grau);
            if (todas) {
                soma += valor;
                maiorGrau = max(maiorGrau, grau);
            }
        }
        if (todas) {
            r.convergiu = true;
            r.valor = soma;
            r.pedacos = partes;
            r.grau = maiorGrau;
            r.coefs.clear();
            return r;
        }
    }
    r.valor = NAN;
    r.coefs.clear();
    return r;
}

// Escreve os primeiros termos não nulos da primitiva obtida pela série:
// F(x) ≈ C + Σ aₖ(x-c)^(k+1)/(k+1)
string formatarPrimitivaSerie(const vector<double>& coefs, double c, int maxTermos) {
    ostringstream base;
    if (fabs(c) < 1e-10) base << "x";
    else base << "(x " << (c > 0 ? "- " : "+ ") << fabs(c) << ")";

    ostringstream oss;
    oss << "F(x) ≈ C";
    int mostrados = 0;
    for (size_t k = 0; k < coefs.size(); k++) {
        double termo = coefs[k] / (k + 1);
        if (fabs(termo) < 1e-12) continue;
        if (mostrados == maxTermos) {
            oss << " + ...";
            break;
        }
        oss << (termo < 0 ? " - " : " + ");
        if (fabs(fabs(termo) - 1.0) > 1e-12) oss << fabs(termo); // "x", não "1x"
        oss << base.str();
        if (k >= 1) oss << superscrito((int)k + 1);
        mostrados++;
    }
    return oss.str();
}

// ---- Avaliação da Série de Taylor ----
// Calcula o valor numérico de P(x) = Σ aₙ(x-a)ⁿ
double avaliarSerieTaylor(const vector<double>& coefs, double a, double x) {
    double resultado = 0.0;  // Acumulador da soma
    double potencia = 1.0;   // (x-a)ⁿ, inicia com (x-a)⁰ = 1

    // Percorre os coeficientes usando iterador constante
    vector<double>::const_iterator it;
    int n = 0;
    for (it = coefs.begin(); it != coefs.end(); ++it, ++n) {
        resultado += (*it) * potencia; // Soma aₙ · (x-a)ⁿ ao acumulador
        potencia *= (x - a);           // Atualiza a potência: (x-a)^(n+1) = (x-a)^n * (x-a)
    }

    return resultado;
}

// ---- Formatação da Série de Taylor (forma de fatorial) ----
// Gera P(x) na forma f(a) + f'(a)(x-a) + [f''(a)/2!](x-a)² + [f'''(a)/3!](x-a)³ + ...
// Mantém o n! explícito no denominador em vez de reduzir o coeficiente a um decimal.
// O numerador exibido é a própria derivada f⁽ⁿ⁾(a) = aₙ · n! (aₙ é o coeficiente recebido).
string formatarSerieTaylor(const vector<double>& coefs, double a) {
    ostringstream oss;                // Stream de string para construir a saída
    oss << fixed << setprecision(4);  // Formata com 4 casas decimais
    oss << "P(x) = ";

    bool primeiro = true; // Flag para controlar o sinal do primeiro termo

    for (int i = 0; i < (int)coefs.size(); i++) {
        // Reconstrói o numerador na forma de fatorial: f⁽ⁱ⁾(a) = aᵢ · i!
        double numerador = coefs[i] * fatorial(i);

        // Ignora termos com numerador desprezível (menor que 10⁻⁶)
        if (abs(numerador) < 1e-6) continue;

        // Gerencia o sinal ('+' ou '-') entre os termos
        if (!primeiro) {
            if (numerador >= 0) oss << " + ";    // Sinal positivo para termos subsequentes
            else { oss << " - "; numerador = -numerador; } // Negativo: mostra '-' e usa o módulo
        } else {
            if (numerador < 0) { oss << "-"; numerador = -numerador; } // Primeiro termo negativo
        }

        if (i == 0) {
            oss << numerador; // Termo constante f(a): apenas o número
        } else {
            // Mostra o numerador (omite se for 1.0, pois 1·x = x)
            if (abs(numerador - 1.0) > 1e-6) oss << numerador;

            // Monta a parte (x-a)ⁱ do termo
            if (abs(a) < 1e-10) {
                oss << "x"; // Maclaurin: apenas "x" (centro em 0)
            } else {
                oss << "(x";
                if (a > 0) oss << " - " << a; // Taylor: "(x - a)"
                else oss << " + " << (-a);      // Taylor: "(x + |a|)"
                oss << ")";
            }

            // Adiciona o expoente em superscrito para grau ≥ 2
            if (i >= 2) oss << superscrito(i);

            // Mantém o fatorial explícito no denominador (1! é omitido por ser 1)
            if (i >= 2) oss << "/" << i << "!";
        }

        primeiro = false; // Após o primeiro termo, ativa os sinais intermediários
    }

    if (primeiro) oss << "0"; // Se todos os coeficientes eram zero

    return oss.str(); // Retorna a string formatada
}

// ---- Resolução de EDO pelo Método de Euler ----
// Resolve dy/dx = g(x, y) com condição inicial y(x₀) = y₀
// Usa a fórmula iterativa: y_{n+1} = y_n + h · g(x_n, y_n)
void resolverEDOEuler(const vector<Token>& f, double x0, double y0, double xf, int numPassos,
                      const string& arquivoCSV) {
    double h = (xf - x0) / numPassos; // Calcula o tamanho do passo

    // Matriz (vetor de vetores) para armazenar a tabela de resultados [x, y]
    vector<vector<double>> tabela;

    double x = x0; // Valor inicial de x
    double y = y0; // Valor inicial de y

    // Armazena a condição inicial como primeira linha da tabela
    vector<double> linhaInicial = {x, y};
    tabela.push_back(linhaInicial);

    // Laço do Método de Euler: avança numPassos vezes
    for (int i = 0; i < numPassos; i++) {
        double dydx = avaliar(f, x, y); // Avalia g(x, y) = dy/dx no ponto atual
        y = y + h * dydx;               // Atualiza y usando a fórmula de Euler
        x = x + h;                       // Avança x pelo passo h

        // Armazena o par (x, y) na tabela
        vector<double> linha = {x, y};
        tabela.push_back(linha);
    }

    // Exibe a tabela de resultados com formatação
    cout << "\n  ┌──────────┬─────────────────┬─────────────────┐" << endl;
    cout << "  │  Passo   │        x        │       y(x)      │" << endl;
    cout << "  ├──────────┼─────────────────┼─────────────────┤" << endl;

    // Calcula o intervalo de exibição para mostrar no máximo 20 linhas
    int intervalo = max(1, (int)tabela.size() / 20);

    // Percorre a matriz usando iterador
    vector<vector<double>>::iterator it;
    int idx = 0;
    for (it = tabela.begin(); it != tabela.end(); ++it, ++idx) {
        // Exibe apenas a cada 'intervalo' passos e sempre o último
        bool ultimoElemento = (it == tabela.end() - 1);   // Verifica se é o último
        bool intervaloAtingido = (idx % intervalo == 0);    // Verifica se está no intervalo

        // Usa operador lógico OR (||) para decidir se exibe a linha
        if (intervaloAtingido || ultimoElemento) {
            cout << "  │ " << setw(8) << idx
                 << " │ " << setw(15) << fixed << setprecision(6) << (*it)[0]
                 << " │ " << setw(15) << (*it)[1] << " │" << endl;
        }
    }

    cout << "  └──────────┴─────────────────┴─────────────────┘" << endl;
    cout << "\n  Resultado final: y(" << xf << ") ≈ " << y << endl;

    // Exporta a tabela COMPLETA (todos os passos) para CSV, se solicitado
    if (!arquivoCSV.empty()) {
        ofstream csv(arquivoCSV);
        if (csv) {
            csv << "passo,x,y\n"; // cabeçalho
            for (size_t i = 0; i < tabela.size(); i++) {
                csv << i << "," << tabela[i][0] << "," << tabela[i][1] << "\n";
            }
            csv.close();
            cout << "  Tabela completa exportada para o arquivo: " << arquivoCSV << endl;
        }
    }
}

// ====================== RAÍZES E EXTREMOS ===================

// ---- Método de Newton-Raphson ----
// Encontra uma raiz de f(x) = 0 a partir do chute inicial x0, iterando
// x_{n+1} = x_n - f(x_n)/f'(x_n). Retorna a raiz ou NAN se não convergir.
// 'iteracoes' (passagem por referência) devolve quantas iterações foram feitas.
double newtonRaphson(const vector<Token>& f, double x0, int& iteracoes) {
    const int MAX_ITER = 100;   // Limite de iterações para evitar laço infinito
    const double TOL = 1e-10;   // Tolerância: |f(x)| abaixo disso é considerado raiz
    double x = x0;

    for (int i = 1; i <= MAX_ITER; i++) {
        double fx = avaliar(f, x);
        if (isnan(fx) || isinf(fx)) { iteracoes = i; return NAN; } // saiu do domínio
        if (fabs(fx) < TOL) { iteracoes = i; return x; }           // convergiu

        double dfx = derivada(f, x);            // f'(x) numérico (diferença central)
        if (fabs(dfx) < 1e-14) { iteracoes = i; return NAN; } // derivada ~0: método falha

        x = x - fx / dfx; // Passo de Newton-Raphson
    }

    iteracoes = MAX_ITER;
    // Aceita o resultado se ficou razoavelmente próximo de uma raiz
    if (fabs(avaliar(f, x)) < 1e-6) return x;
    return NAN; // Não convergiu
}

// ---- Pontos Críticos (máximos, mínimos e inflexões) ----
// f'(x) pela aritmética de séries: sem o ruído das diferenças finitas
static double derivadaExata(const vector<Token>& f, double x) {
    return calcularCoefsTaylor(f, x, 2)[1];
}

// +1, -1, ou 0 (também para NAN)
static int sinalDe(double v) {
    return (v > 0) - (v < 0);
}

// True se f está definida e é contínua em x (descarta polos, como 1/x² em 0)
static bool continuaEm(const vector<Token>& f, double x) {
    double v = avaliar(f, x);
    if (!finito(v)) return false;
    double d = 1e-9 * max(1.0, fabs(x));
    double tol = 1e-3 * max(1.0, fabs(v));
    double esq = avaliar(f, x - d), dir = avaliar(f, x + d);
    return !(finito(esq) && fabs(esq - v) > tol) && !(finito(dir) && fabs(dir - v) > tol);
}

// Bisseção sobre f' entre dois pontos em que ela tem sinais opostos
static double bissecaoDerivada(const vector<Token>& f, double lo, double hi) {
    int sinalLo = sinalDe(derivadaExata(f, lo));
    for (int k = 0; k < 80; k++) {
        double m = (lo + hi) / 2.0;
        int s = sinalDe(derivadaExata(f, m));
        if (s == 0) return m; // raiz exata, ou bico em que f' não existe
        if (s == sinalLo) lo = m;
        else hi = m;
    }
    return (lo + hi) / 2.0;
}

// Busca, por seção áurea, o ponto de [lo, hi] em que |f'| é mínimo
static double minimoModuloDerivada(const vector<Token>& f, double lo, double hi) {
    const double R = 0.6180339887498949;
    for (int i = 0; i < 80; i++) {
        double c = hi - R * (hi - lo), d = lo + R * (hi - lo);
        if (fabs(derivadaExata(f, c)) <= fabs(derivadaExata(f, d))) hi = d;
        else lo = c;
    }
    return (lo + hi) / 2.0;
}

// Procura em [a, b] os pontos onde f'(x) = 0 (ou não existe) e classifica cada um
// pelo teste da primeira derivada:
//   f' passa de - para + → mínimo | de + para - → máximo
//   f' não troca de sinal → inflexão com tangente horizontal
void acharPontosCriticos(const vector<Token>& f, double a, double b) {
    if (a > b) swap(a, b);
    cout << "\n  ── Pontos Críticos em [" << a << ", " << b << "] ──" << endl;
    if (!(a < b) || !finito(b - a)) {
        cout << "  Intervalo inválido: use dois valores diferentes." << endl;
        return;
    }

    const int N = 2000;                 // Subdivisões da varredura
    double passo = (b - a) / N;

    // Amostra f' também um passo antes de a e um depois de b, para enxergar
    // pontos críticos que estejam exatamente nas pontas
    vector<double> x, d;
    double maiorModulo = 0.0;
    for (int i = -1; i <= N + 1; i++) {
        x.push_back(a + i * passo);
        d.push_back(derivadaExata(f, x.back()));
        if (fabs(d.back()) > maiorModulo) maiorModulo = fabs(d.back());
    }
    if (maiorModulo <= 1e-12) {
        cout << "  f'(x) = 0 em todo o intervalo: f é constante." << endl;
        return;
    }

    vector<double> candidatos;
    int trechosConstantes = 0;
    int ultimo = -1; // último índice com sinal definido de f'

    for (size_t i = 0; i < x.size(); i++) {
        if (d[i] == 0.0) {
            // Sequência de zeros: um ponto crítico ou um trecho constante
            size_t j = i;
            while (j + 1 < x.size() && d[j + 1] == 0.0) j++;
            if (j - i >= 2) {
                cout << "  f'(x) = 0 em todo o trecho [" << max(a, x[i]) << ", "
                     << min(b, x[j]) << "]: f é constante ali." << endl;
                trechosConstantes++;
                ultimo = -1;
            } else {
                candidatos.push_back(x[i]);
            }
            i = j;
        } else if (isnan(d[i])) {
            // Fora do domínio interrompe a varredura; um bico (f definida) não
            if (!finito(avaliar(f, x[i]))) ultimo = -1;
        } else {
            if (ultimo >= 0 && sinalDe(d[i]) != sinalDe(d[ultimo])) {
                candidatos.push_back(bissecaoDerivada(f, x[ultimo], x[i]));
            }
            ultimo = (int)i;
        }
    }

    // Zeros em que f' encosta no eixo sem trocar de sinal (ex.: x³ em 0)
    for (size_t i = 1; i + 1 < x.size(); i++) {
        int s = sinalDe(d[i]);
        if (s != 0 && sinalDe(d[i - 1]) == s && sinalDe(d[i + 1]) == s &&
            fabs(d[i]) < fabs(d[i - 1]) && fabs(d[i]) <= fabs(d[i + 1])) {
            double r = minimoModuloDerivada(f, x[i - 1], x[i + 1]);
            if (fabs(derivadaExata(f, r)) <= 1e-9) candidatos.push_back(r);
        }
    }

    // Mantém os candidatos dentro de [a, b], sem repetições e onde f é contínua
    sort(candidatos.begin(), candidatos.end());
    double junta = min(passo / 2.0, 1e-4);
    vector<double> criticos;
    for (size_t i = 0, j = 0; i < candidatos.size(); i = j + 1) {
        // Candidatos quase colados são o mesmo ponto (ruído de arredondamento
        // em torno de uma raiz múltipla): usa o meio do grupo
        j = i;
        while (j + 1 < candidatos.size() && candidatos[j + 1] - candidatos[j] < junta) j++;
        double r = (candidatos[i] + candidatos[j]) / 2.0;
        double folga = 1e-9 * max(1.0, fabs(r));
        if (r < a - folga || r > b + folga) continue;
        if (!continuaEm(f, r)) continue;
        criticos.push_back(r);
    }

    for (size_t i = 0; i < criticos.size(); i++) {
        double r = criticos[i];
        double delta = passo / 2.0; // distância para olhar o sinal de cada lado
        if (i > 0) delta = min(delta, (r - criticos[i - 1]) / 2.0);
        if (i + 1 < criticos.size()) delta = min(delta, (criticos[i + 1] - r) / 2.0);
        int esq = sinalDe(derivadaExata(f, r - delta));
        int dir = sinalDe(derivadaExata(f, r + delta));
        double dr = derivadaExata(f, r);
        bool bico = isnan(dr) || fabs(dr) > 1e-3;

        if (fabs(r) < 1e-9) r = 0.0;                   // evita "-0.000000"
        double fr = avaliar(f, r);
        if (fabs(fr) < 5e-7) fr = 0.0;
        cout << "  x = " << fixed << setprecision(6) << r
             << "  | f(x) = " << fr << "  → ";
        if (esq < 0 && dir > 0)      cout << "MÍNIMO local";
        else if (esq > 0 && dir < 0) cout << "MÁXIMO local";
        else if (esq * dir > 0)      cout << "inflexão com tangente horizontal";
        else                         cout << "ponto crítico na borda do domínio ou de trecho constante";
        if (bico) cout << " (f' não existe)";
        cout << endl;
    }

    if (criticos.empty() && trechosConstantes == 0) {
        cout << "  Nenhum ponto crítico encontrado no intervalo." << endl;
    }
}

// ================ MANIPULAÇÃO DE ARQUIVOS (BINÁRIO) =========

// Salva os coeficientes de Taylor em um arquivo BINÁRIO.
// Formato: [int n] [double centro] [n doubles com os coeficientes].
// Usa ponteiros (cast para char*) e ofstream em modo ios::binary.
bool salvarCoeficientesBin(const string& nomeArquivo, const vector<double>& coefs, double centro) {
    ofstream arq(nomeArquivo, ios::binary);
    if (!arq) return false; // não conseguiu abrir o arquivo

    int n = (int)coefs.size();
    arq.write((const char*)&n, sizeof(int));        // grava a quantidade
    arq.write((const char*)&centro, sizeof(double)); // grava o centro
    for (int i = 0; i < n; i++) {
        double v = coefs[i];
        arq.write((const char*)&v, sizeof(double));  // grava cada coeficiente
    }
    arq.close();
    return true;
}

// Lê de volta os coeficientes de Taylor de um arquivo binário gravado acima.
bool carregarCoeficientesBin(const string& nomeArquivo, vector<double>& coefs, double& centro) {
    ifstream arq(nomeArquivo, ios::binary);
    if (!arq) return false; // arquivo não existe

    int n = 0;
    arq.read((char*)&n, sizeof(int));
    if (!arq || n <= 0 || n > 100000) return false; // proteção contra arquivo inválido
    arq.read((char*)&centro, sizeof(double));

    coefs.clear();
    for (int i = 0; i < n; i++) {
        double v = 0.0;
        arq.read((char*)&v, sizeof(double));
        if (!arq) return false; // arquivo truncado
        coefs.push_back(v);
    }
    arq.close();
    return true;
}

// ============ CÁLCULO 2: DERIVADAS PARCIAIS E SÉRIES ========

// ---- Derivada Parcial (SOBRECARGA de derivada) ----
// Esta função tem o MESMO NOME de derivada(f, x), mas uma assinatura de entrada
// diferente — é um exemplo de SOBRECARGA de funções (notas de aula, seção 2.6).
// Calcula a derivada parcial ∂f/∂var no ponto (x, y), tratando a outra variável
// como constante, via diferença central.
double derivada(const vector<Token>& f, double x, double y, char var) {
    double h = 1e-7; // incremento infinitesimal
    if (var == 'y') {
        // ∂f/∂y: varia y, mantém x fixo
        return (avaliar(f, x, y + h) - avaliar(f, x, y - h)) / (2.0 * h);
    }
    // ∂f/∂x: varia x, mantém y fixo
    return (avaliar(f, x + h, y) - avaliar(f, x - h, y)) / (2.0 * h);
}

// ---- Teste do Termo Geral ----
// Maior |a(k)| numa janela de 20 termos a partir de n. A janela evita que um termo
// que oscila, como sin(n), pareça pequeno só por ter sido amostrado perto de um zero.
static double picoTermo(const vector<Token>& termo, double n) {
    double pico = 0.0;
    for (int k = 0; k < 20; k++) {
        double v = fabs(avaliar(termo, n + k));
        if (isnan(v)) return NAN;
        if (v > pico) pico = v;
    }
    return pico;
}

// True se |a(n)| praticamente não diminui entre n = 10⁶ e 10⁷: indício de que a(n)
// não tende a 0 e a série diverge. A comparação é relativa, então a escala do termo
// (ex.: 10⁷/n²) não interfere.
bool termoNaoTendeAZero(const vector<Token>& termo) {
    double antes = picoTermo(termo, 1e6);
    double depois = picoTermo(termo, 1e7);
    return depois > 0 && depois >= 0.99 * antes;
}

// ---- Teste da Razão (d'Alembert) ----
// Calcula L = lim |a(n+1)/a(n)| para n grande. Numa série Σa(n):
//   L < 1 → converge | L > 1 → diverge | L = 1 → inconclusivo.
double testeRazaoSerie(const vector<Token>& termo) {
    double ns[] = {50.0, 100.0, 300.0, 800.0}; // n crescentes
    double L = NAN;
    double nL = 0.0; // n em que L foi calculado
    for (int i = 0; i < 4; i++) {
        double an  = avaliar(termo, ns[i]);
        double an1 = avaliar(termo, ns[i] + 1.0);
        if (!isnan(an) && !isnan(an1) && fabs(an) > 1e-300) {
            L = fabs(an1 / an); // guarda a razão no maior n válido
            nL = ns[i];
        }
    }
    if (!isnan(L)) {
        // O teste exige que a razão tenha limite: se a razão seguinte é bem
        // diferente (ex.: termo com sin(n)), ela oscila e o teste não se aplica
        double seguinte = fabs(avaliar(termo, nL + 2.0) / avaliar(termo, nL + 1.0));
        if (!(fabs(seguinte - L) <= 0.01 * L)) return NAN;
    }
    return L;
}

// True se, numa janela a partir de n, os termos alternam de sinal e |a(k)| não cresce
static bool alternadaDecrescente(const vector<Token>& termo, double n) {
    for (int k = 0; k < 20; k++) {
        double atual = avaliar(termo, n + k);
        double prox  = avaliar(termo, n + k + 1);
        bool alterna = (atual > 0 && prox < 0) || (atual < 0 && prox > 0);
        if (!alterna || fabs(prox) > fabs(atual)) return false;
    }
    return true;
}

// ---- Análise do Decaimento ----
// Usada quando a(n) → 0 e o teste da razão não decide. Aplica, nesta ordem:
// razão constante (série geométrica), Leibniz (série alternada) e comparação com a
// série-p, estimando p em |a(n)| ~ C/n^p. Só conclui quando a estimativa é segura;
// fatores logarítmicos, como em 1/(n·ln(n)), ficam inconclusivos.
Veredito analisarDecaimento(const vector<Token>& termo, string& motivo) {
    double r300 = fabs(avaliar(termo, 301.0) / avaliar(termo, 300.0));
    double r800 = fabs(avaliar(termo, 801.0) / avaliar(termo, 800.0));
    if (r800 < 1.0 - 1e-9 && fabs(r800 - r300) < 1e-9) {
        motivo = "razão constante menor que 1 (decaimento geométrico).";
        return SERIE_CONVERGE;
    }

    if (alternadaDecrescente(termo, 1e3) && alternadaDecrescente(termo, 1e5)) {
        motivo = "série alternada com |a(n)| decrescendo até 0 (critério de Leibniz).";
        return SERIE_CONVERGE;
    }

    double m5 = picoTermo(termo, 1e5);
    double m6 = picoTermo(termo, 1e6);
    double m7 = picoTermo(termo, 1e7);
    if (m5 == 0 && m6 == 0 && m7 == 0) {
        motivo = "os termos zeram mais rápido que qualquer potência de n.";
        return SERIE_CONVERGE;
    }
    if (!(m5 > 0 && m6 > 0 && m7 > 0)) {
        motivo = "não foi possível calcular os termos para n grande.";
        return SERIE_INCONCLUSIVA;
    }

    double p1 = log10(m5 / m6); // expoente estimado entre 10⁵ e 10⁶
    double p2 = log10(m6 / m7); // expoente estimado entre 10⁶ e 10⁷
    ostringstream oss;
    oss << "|a(n)| se comporta como 1/n^p, com p ≈ " << fixed << setprecision(3) << p2;

    // p bem acima de 1 e sem queda: converge (comparação, vale para qualquer sinal)
    if (p2 > 1.25 && p2 >= p1 - 0.1) {
        motivo = oss.str() + " > 1.";
        return SERIE_CONVERGE;
    }

    // Perto de p = 1, ou abaixo, só se decide para termos de sinal constante e
    // |a(n)| decrescente: se oscilam, a estimativa de p não é confiável
    bool positivos = true, negativos = true, decrescente = true;
    for (int k = 0; k < 20; k++) {
        double v = avaliar(termo, 1e5 + k);
        if (!(v > 0)) positivos = false;
        if (!(v < 0)) negativos = false;
        if (fabs(avaliar(termo, 1e5 + k + 1)) > fabs(v)) decrescente = false;
    }
    if ((positivos || negativos) && decrescente) {
        double variacao = fabs(p1 - p2);
        if (variacao < 0.001) { // p estável
            // Só converge se a distância até 1 for bem maior que a variação de p;
            // caso contrário p está indo para 1, como na série harmônica
            if (p2 - 1.0 > 10.0 * variacao + 1e-6) {
                motivo = oss.str() + " > 1.";
                return SERIE_CONVERGE;
            }
            motivo = "|a(n)| se comporta como 1/n^p, com p tendendo a 1 ou menor.";
            return SERIE_DIVERGE;
        }
        if (p2 < 0.75 && p2 <= p1 + 0.1) {
            motivo = oss.str() + " < 1.";
            return SERIE_DIVERGE;
        }
    }

    motivo = oss.str() + ", o que não basta para decidir (tente o teste da integral).";
    return SERIE_INCONCLUSIVA;
}

// ---- Soma Parcial de uma Série ----
// Calcula S(n) = a(1) + a(2) + ... + a(n), onde a(k) = termo avaliado em x = k.
double somaParcialSerie(const vector<Token>& termo, int n) {
    double soma = 0.0;
    for (int k = 1; k <= n; k++) {
        double ak = avaliar(termo, (double)k);
        if (isnan(ak) || isinf(ak)) continue; // ex.: 1/(n·ln(n)) em n = 1
        soma += ak;
    }
    return soma;
}

// ================ DIFERENCIAÇÃO SIMBÓLICA ====================

// ---- Construtores de Nó ----
// Cada método estático cria um nó de um tipo específico da árvore

No* No::criarNumero(double v) {
    No* n = new No();          // Aloca novo nó na memória dinâmica (heap)
    n->tipoNo = NUM_NO;        // Define o tipo como número
    n->valor = v;              // Armazena o valor numérico
    return n;
}

No* No::criarVariavel(char c) {
    No* n = new No();
    n->tipoNo = VAR_NO;        // Define o tipo como variável
    n->variavel = c;           // Armazena 'x' ou 'y'
    return n;
}

No* No::criarOperacao(char op, No* esq, No* dir) {
    No* n = new No();
    n->tipoNo = OP_NO;         // Define o tipo como operador
    n->operador = op;          // Armazena +, -, *, / ou ^
    n->esquerda = esq;         // Conecta o filho esquerdo (primeiro operando)
    n->direita = dir;          // Conecta o filho direito (segundo operando)
    return n;
}

No* No::criarFuncao(const string& nome, No* arg) {
    No* n = new No();
    n->tipoNo = FUNC_NO;       // Define o tipo como função
    n->funcao = nome;          // Armazena o nome (sin, cos, ln, etc.)
    n->esquerda = arg;         // O argumento da função é o filho esquerdo
    return n;
}

// ---- Construir Árvore ----
// Monta a árvore de expressão a partir dos tokens em notação pós-fixa
// Usa uma pilha de ponteiros: números/variáveis são empilhados,
// operadores desempilham dois filhos, funções desempilham um filho
No* construirArvore(const vector<Token>& posfixo) {
    stack<No*> pilha;           // Pilha de ponteiros para nós da árvore

    for (size_t i = 0; i < posfixo.size(); i++) {
        const Token& tok = posfixo[i]; // Token atual

        switch (tok.tipo) {
            case Token::NUMERO:
                pilha.push(No::criarNumero(tok.valor)); // Empilha nó folha de número
                break;

            case Token::VARIAVEL:
                pilha.push(No::criarVariavel(tok.variavel)); // Empilha nó folha de variável
                break;

            case Token::OPERADOR: {
                // Na árvore não há nó unário: -u vira número negativo ou (-1)*u,
                // formas que derivação, integração e simplificação já tratam
                if (tok.operador == OP_NEGACAO) {
                    if (pilha.empty()) return No::criarNumero(0); // Proteção
                    No* u = pilha.top(); pilha.pop();
                    if (u->tipoNo == No::NUM_NO) {
                        u->valor = -u->valor;
                        pilha.push(u);
                    } else {
                        pilha.push(No::criarOperacao('*', No::criarNumero(-1), u));
                    }
                    break;
                }
                if (pilha.size() < 2) return No::criarNumero(0); // Proteção contra expressão inválida
                No* dir = pilha.top(); pilha.pop();  // Desempilha filho direito
                No* esq = pilha.top(); pilha.pop();  // Desempilha filho esquerdo
                pilha.push(No::criarOperacao(tok.operador, esq, dir)); // Cria nó de operação
                break;
            }

            case Token::FUNCAO: {
                if (pilha.empty()) return No::criarNumero(0); // Proteção
                No* arg = pilha.top(); pilha.pop();  // Desempilha o argumento
                pilha.push(No::criarFuncao(tok.funcao, arg)); // Cria nó de função
                break;
            }

            default:
                break;
        }
    }

    return pilha.empty() ? No::criarNumero(0) : pilha.top(); // A raiz é o último nó na pilha
}

// ---- Cópia Profunda ----
// Cria uma cópia recursiva de toda a sub-árvore (necessário para regras de derivação)
No* copiar(No* no) {
    if (!no) return nullptr;    // Ponteiro nulo: retorna nulo

    No* copia = new No();      // Aloca novo nó
    copia->tipoNo = no->tipoNo;
    copia->valor = no->valor;
    copia->operador = no->operador;
    copia->variavel = no->variavel;
    copia->funcao = no->funcao;
    copia->esquerda = copiar(no->esquerda); // Copia recursivamente o filho esquerdo
    copia->direita = copiar(no->direita);   // Copia recursivamente o filho direito
    return copia;
}

// ---- Verificações Auxiliares ----

// Verifica se um nó representa uma expressão constante (sem variáveis)
bool ehConstante(No* no) {
    if (!no) return true;
    if (no->tipoNo == No::NUM_NO) return true;       // Número é constante
    if (no->tipoNo == No::VAR_NO) return false;       // Variável não é constante
    if (no->tipoNo == No::OP_NO)                      // Operação é constante se ambos filhos são
        return ehConstante(no->esquerda) && ehConstante(no->direita);
    if (no->tipoNo == No::FUNC_NO)                    // Função é constante se o argumento é
        return ehConstante(no->esquerda);
    return false;
}

// Verifica se o nó é o número zero
bool ehZero(No* no) {
    return no && no->tipoNo == No::NUM_NO && abs(no->valor) < 1e-10;
}

// Verifica se o nó é o número um
bool ehUm(No* no) {
    return no && no->tipoNo == No::NUM_NO && abs(no->valor - 1.0) < 1e-10;
}

// Avalia uma sub-árvore que contém apenas constantes
double avaliarConstanteNo(No* no) {
    if (!no) return 0;
    if (no->tipoNo == No::NUM_NO) return no->valor;
    if (no->tipoNo == No::OP_NO) {
        double a = avaliarConstanteNo(no->esquerda);
        double b = avaliarConstanteNo(no->direita);
        switch (no->operador) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/': return b != 0 ? a / b : NAN;
            case '^': return pow(a, b);
        }
    }
    if (no->tipoNo == No::FUNC_NO) {
        return avaliarFuncao(no->funcao, avaliarConstanteNo(no->esquerda));
    }
    return 0;
}

// ---- Diferenciação Simbólica ----
// Aplica as regras de derivação recursivamente sobre a árvore
// Retorna uma NOVA árvore representando f'(x)
No* diferenciar(No* no, char var) {
    if (!no) return No::criarNumero(0);

    switch (no->tipoNo) {

        // d/dx(constante) = 0
        case No::NUM_NO:
            return No::criarNumero(0);

        // d/dx(x) = 1,  d/dx(y) = 0
        case No::VAR_NO:
            return No::criarNumero(no->variavel == var ? 1 : 0);

        // Regras para operadores
        case No::OP_NO: {
            No* f = no->esquerda;               // Primeiro operando
            No* g = no->direita;                // Segundo operando

            switch (no->operador) {

                // d/dx(f + g) = f' + g'  — regra da soma
                case '+':
                    return No::criarOperacao('+', diferenciar(f, var), diferenciar(g, var));

                // d/dx(f - g) = f' - g'  — regra da diferença
                case '-':
                    return No::criarOperacao('-', diferenciar(f, var), diferenciar(g, var));

                // d/dx(f · g) = f'·g + f·g'  — regra do produto
                case '*':
                    return No::criarOperacao('+',
                        No::criarOperacao('*', diferenciar(f, var), copiar(g)),
                        No::criarOperacao('*', copiar(f), diferenciar(g, var)));

                // d/dx(f / g) = (f'·g - f·g') / g²  — regra do quociente
                case '/':
                    return No::criarOperacao('/',
                        No::criarOperacao('-',
                            No::criarOperacao('*', diferenciar(f, var), copiar(g)),
                            No::criarOperacao('*', copiar(f), diferenciar(g, var))),
                        No::criarOperacao('^', copiar(g), No::criarNumero(2)));

                // d/dx(f ^ g) — regra da potência / exponencial
                case '^': {
                    bool fConst = ehConstante(f);
                    bool gConst = ehConstante(g);

                    // Ambos constantes: derivada é 0
                    if (fConst && gConst) return No::criarNumero(0);

                    // Expoente constante: d/dx(f^n) = n · f^(n-1) · f'  — regra da potência
                    if (gConst) {
                        return No::criarOperacao('*',
                            No::criarOperacao('*',
                                copiar(g),
                                No::criarOperacao('^', copiar(f),
                                    No::criarOperacao('-', copiar(g), No::criarNumero(1)))),
                            diferenciar(f, var));
                    }

                    // Base constante: d/dx(a^g) = a^g · ln(a) · g'  — regra exponencial
                    if (fConst) {
                        return No::criarOperacao('*',
                            No::criarOperacao('*',
                                No::criarOperacao('^', copiar(f), copiar(g)),
                                No::criarFuncao("ln", copiar(f))),
                            diferenciar(g, var));
                    }

                    // Caso geral: d/dx(f^g) = f^g · (g'·ln(f) + g·f'/f)
                    return No::criarOperacao('*',
                        No::criarOperacao('^', copiar(f), copiar(g)),
                        No::criarOperacao('+',
                            No::criarOperacao('*', diferenciar(g, var),
                                No::criarFuncao("ln", copiar(f))),
                            No::criarOperacao('*', copiar(g),
                                No::criarOperacao('/', diferenciar(f, var), copiar(f)))));
                }
            }
            break;
        }

        // Regra da cadeia para funções: d/dx[h(u)] = h'(u) · u'
        case No::FUNC_NO: {
            No* u = no->esquerda;               // Argumento da função
            No* du = diferenciar(u, var);        // Derivada do argumento (u')
            string nome = no->funcao;

            No* derivExterna = nullptr;          // h'(u) — derivada da função externa

            // d/dx(sin(u)) = cos(u) · u'
            if (nome == "sin") {
                derivExterna = No::criarFuncao("cos", copiar(u));
            }
            // d/dx(cos(u)) = -sin(u) · u'
            else if (nome == "cos") {
                derivExterna = No::criarOperacao('*', No::criarNumero(-1),
                    No::criarFuncao("sin", copiar(u)));
            }
            // d/dx(tan(u)) = (1/cos²(u)) · u'
            else if (nome == "tan") {
                derivExterna = No::criarOperacao('/',
                    No::criarNumero(1),
                    No::criarOperacao('^', No::criarFuncao("cos", copiar(u)), No::criarNumero(2)));
            }
            // d/dx(ln(u)) = (1/u) · u'
            else if (nome == "ln") {
                derivExterna = No::criarOperacao('/', No::criarNumero(1), copiar(u));
            }
            // d/dx(log₁₀(u)) = 1/(u·ln(10)) · u'
            else if (nome == "log") {
                derivExterna = No::criarOperacao('/',
                    No::criarNumero(1),
                    No::criarOperacao('*', copiar(u),
                        No::criarFuncao("ln", No::criarNumero(10))));
            }
            // d/dx(eᵘ) = eᵘ · u'
            else if (nome == "exp") {
                derivExterna = No::criarFuncao("exp", copiar(u));
            }
            // d/dx(√u) = 1/(2√u) · u'
            else if (nome == "sqrt") {
                derivExterna = No::criarOperacao('/',
                    No::criarNumero(1),
                    No::criarOperacao('*', No::criarNumero(2),
                        No::criarFuncao("sqrt", copiar(u))));
            }
            // d/dx(asin(u)) = 1/√(1-u²) · u'
            else if (nome == "asin") {
                derivExterna = No::criarOperacao('/',
                    No::criarNumero(1),
                    No::criarFuncao("sqrt", No::criarOperacao('-', No::criarNumero(1),
                        No::criarOperacao('^', copiar(u), No::criarNumero(2)))));
            }
            // d/dx(acos(u)) = -1/√(1-u²) · u'
            else if (nome == "acos") {
                derivExterna = No::criarOperacao('/',
                    No::criarNumero(-1),
                    No::criarFuncao("sqrt", No::criarOperacao('-', No::criarNumero(1),
                        No::criarOperacao('^', copiar(u), No::criarNumero(2)))));
            }
            // d/dx(atan(u)) = 1/(1+u²) · u'
            else if (nome == "atan") {
                derivExterna = No::criarOperacao('/',
                    No::criarNumero(1),
                    No::criarOperacao('+', No::criarNumero(1),
                        No::criarOperacao('^', copiar(u), No::criarNumero(2))));
            }
            // d/dx(sec(u)) = sec(u) * tan(u) * u'
            else if (nome == "sec") {
                derivExterna = No::criarOperacao('*',
                    No::criarFuncao("sec", copiar(u)),
                    No::criarFuncao("tan", copiar(u)));
            }
            // d/dx(csc(u)) = -csc(u) * cot(u) * u'
            else if (nome == "csc") {
                derivExterna = No::criarOperacao('*',
                    No::criarNumero(-1),
                    No::criarOperacao('*',
                        No::criarFuncao("csc", copiar(u)),
                        No::criarFuncao("cot", copiar(u))));
            }
            // d/dx(cot(u)) = -csc²(u) * u'
            else if (nome == "cot") {
                derivExterna = No::criarOperacao('*',
                    No::criarNumero(-1),
                    No::criarOperacao('^', No::criarFuncao("csc", copiar(u)), No::criarNumero(2)));
            }
            // d/dx(abs(u)) = (u / abs(u)) * u'
            else if (nome == "abs") {
                derivExterna = No::criarOperacao('/', copiar(u), No::criarFuncao("abs", copiar(u)));
            }
            // d/dx(sinh(u)) = cosh(u) * u'
            else if (nome == "sinh") {
                derivExterna = No::criarFuncao("cosh", copiar(u));
            }
            // d/dx(cosh(u)) = sinh(u) * u'
            else if (nome == "cosh") {
                derivExterna = No::criarFuncao("sinh", copiar(u));
            }
            // d/dx(tanh(u)) = sech²(u) * u' = (1/cosh²(u)) * u'
            else if (nome == "tanh") {
                derivExterna = No::criarOperacao('/',
                    No::criarNumero(1),
                    No::criarOperacao('^', No::criarFuncao("cosh", copiar(u)), No::criarNumero(2)));
            }
            // Função desconhecida: retorna 0
            else {
                return No::criarNumero(0);
            }

            // Aplica a regra da cadeia: h'(u) * u'
            return No::criarOperacao('*', derivExterna, du);
        }
    }

    return No::criarNumero(0);
}

// ============== AUXILIARES DE INTEGRAÇÃO ====================
// Funções de pattern matching usadas por integrar() para reconhecer
// padrões integráveis: substituição linear, u-substituição (substituição
// simples), integração por partes e monômios

// Avalia uma árvore numericamente em um ponto t da variável var
// Variáveis diferentes de var retornam NaN de propósito: isso faz os
// testes numéricos de proporcionalidade rejeitarem expressões em y
double avaliarNo(No* no, char var, double t) {
    if (!no) return NAN;
    switch (no->tipoNo) {
        case No::NUM_NO: return no->valor;
        case No::VAR_NO: return no->variavel == var ? t : NAN;
        case No::OP_NO: {
            double a = avaliarNo(no->esquerda, var, t);
            double b = avaliarNo(no->direita, var, t);
            switch (no->operador) {
                case '+': return a + b;
                case '-': return a - b;
                case '*': return a * b;
                case '/': return b != 0.0 ? a / b : NAN;
                case '^': return pow(a, b);
            }
            return NAN;
        }
        case No::FUNC_NO:
            return avaliarFuncao(no->funcao, avaliarNo(no->esquerda, var, t));
    }
    return NAN;
}

// Aplica simplificar() várias vezes (mesmo padrão usado no programa principal)
No* simplificarVarias(No* no) {
    for (int s = 0; s < 5; s++) no = simplificar(no);
    return no;
}

// Testa se a(x)/b(x) é uma razão constante k, avaliando em pontos de amostra
// É o coração da substituição simples: detecta se um fator é múltiplo de u'
bool razaoConstante(No* a, No* b, char var, double& k) {
    const double amostras[] = {0.317, 0.563, 0.789, 1.137, 1.523};
    const int N = 5;
    int validas = 0;
    k = 0.0;

    for (int i = 0; i < N; i++) {
        double va = avaliarNo(a, var, amostras[i]);
        double vb = avaliarNo(b, var, amostras[i]);
        // Ignora pontos onde alguma das expressões não está definida
        if (isnan(va) || isinf(va) || isnan(vb) || isinf(vb) || abs(vb) < 1e-9) continue;

        double razao = va / vb;
        if (validas == 0) k = razao;                                      // Primeira razão vira referência
        else if (abs(razao - k) > 1e-6 * max(1.0, abs(k))) return false; // Razões divergem: não é múltiplo
        validas++;
    }

    if (validas < 3 || abs(k) < 1e-9) return false; // Amostras insuficientes ou razão nula

    if (abs(k - round(k)) < 1e-9) k = round(k); // Limpa ruído numérico em razões inteiras
    return true;
}

// Verifica se a sub-árvore é linear na variável: no = a·x + b
// Preenche os coeficientes a e b por recursão; retorna false caso contrário
bool ehLinear(No* no, char var, double& a, double& b) {
    if (!no) return false;

    // Constante pura: a = 0, b = valor
    if (ehConstante(no)) {
        a = 0.0;
        b = avaliarConstanteNo(no);
        return !isnan(b) && !isinf(b);
    }

    // A própria variável: a = 1, b = 0
    if (no->tipoNo == No::VAR_NO) {
        if (no->variavel != var) return false;
        a = 1.0; b = 0.0;
        return true;
    }

    if (no->tipoNo == No::OP_NO) {
        double a1, b1, a2, b2;
        switch (no->operador) {
            case '+': // (a1·x + b1) + (a2·x + b2)
                if (ehLinear(no->esquerda, var, a1, b1) && ehLinear(no->direita, var, a2, b2)) {
                    a = a1 + a2; b = b1 + b2;
                    return true;
                }
                return false;
            case '-': // (a1·x + b1) - (a2·x + b2)
                if (ehLinear(no->esquerda, var, a1, b1) && ehLinear(no->direita, var, a2, b2)) {
                    a = a1 - a2; b = b1 - b2;
                    return true;
                }
                return false;
            case '*': // constante × linear (em qualquer ordem)
                if (ehConstante(no->esquerda) && ehLinear(no->direita, var, a2, b2)) {
                    double c = avaliarConstanteNo(no->esquerda);
                    a = c * a2; b = c * b2;
                    return true;
                }
                if (ehConstante(no->direita) && ehLinear(no->esquerda, var, a1, b1)) {
                    double c = avaliarConstanteNo(no->direita);
                    a = c * a1; b = c * b1;
                    return true;
                }
                return false;
            case '/': // linear ÷ constante
                if (ehConstante(no->direita) && ehLinear(no->esquerda, var, a1, b1)) {
                    double c = avaliarConstanteNo(no->direita);
                    if (abs(c) < 1e-12) return false;
                    a = a1 / c; b = b1 / c;
                    return true;
                }
                return false;
        }
    }
    return false;
}

// Reconhece monômios c·x^n, incluindo formas compostas como x*x, x²·3x e x³/2
// Preenche coeficiente e grau; retorna false se não for monômio
bool ehMonomio(No* no, char var, double& coef, double& grau) {
    if (!no) return false;

    if (ehConstante(no)) { // Constante: grau 0
        coef = avaliarConstanteNo(no);
        grau = 0.0;
        return !isnan(coef) && !isinf(coef);
    }
    if (no->tipoNo == No::VAR_NO) { // x puro: grau 1
        if (no->variavel != var) return false;
        coef = 1.0; grau = 1.0;
        return true;
    }
    if (no->tipoNo == No::OP_NO) {
        double c1, g1, c2, g2;
        switch (no->operador) {
            case '*': // Produto soma os graus: x²·x = x³
                if (ehMonomio(no->esquerda, var, c1, g1) && ehMonomio(no->direita, var, c2, g2)) {
                    coef = c1 * c2; grau = g1 + g2;
                    return true;
                }
                return false;
            case '/': // Divisão subtrai os graus: x³/x = x²
                if (ehMonomio(no->esquerda, var, c1, g1) && ehMonomio(no->direita, var, c2, g2) &&
                    abs(c2) > 1e-12) {
                    coef = c1 / c2; grau = g1 - g2;
                    return true;
                }
                return false;
            case '^': // Potência multiplica o grau: (x²)³ = x⁶
                if (ehMonomio(no->esquerda, var, c1, g1) && ehConstante(no->direita)) {
                    double n = avaliarConstanteNo(no->direita);
                    if (isnan(n) || isinf(n)) return false;
                    coef = pow(c1, n); grau = g1 * n;
                    return !isnan(coef) && !isinf(coef);
                }
                return false;
        }
    }
    return false;
}

// Monta a árvore de ∫c·x^g dx: c·x^(g+1)/(g+1), ou c·ln(x) quando g = -1
No* integrarMonomio(double coef, double grau, char var) {
    No* resultado;
    if (abs(grau + 1.0) < 1e-10) {
        resultado = No::criarFuncao("ln", No::criarVariavel(var)); // ∫x⁻¹dx = ln(x)
    } else {
        resultado = No::criarOperacao('/',
            No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(grau + 1)),
            No::criarNumero(grau + 1));
    }
    if (abs(coef - 1.0) < 1e-12) return resultado;
    return No::criarOperacao('*', No::criarNumero(coef), resultado);
}

// Separa o fator constante de uma expressão: no = c · resto
// Ex.: (-1)·cos(2x)/2  →  c = -0.5 e resto = cos(2x)
void separarConstante(No* no, double& c, No*& resto) {
    if (no && no->tipoNo == No::OP_NO && no->operador == '*') {
        double c2; No* r2;
        if (ehConstante(no->esquerda)) {
            separarConstante(no->direita, c2, r2);
            c = avaliarConstanteNo(no->esquerda) * c2;
            resto = r2;
            return;
        }
        if (ehConstante(no->direita)) {
            separarConstante(no->esquerda, c2, r2);
            c = avaliarConstanteNo(no->direita) * c2;
            resto = r2;
            return;
        }
    }
    if (no && no->tipoNo == No::OP_NO && no->operador == '/' && ehConstante(no->direita)) {
        double den = avaliarConstanteNo(no->direita);
        if (abs(den) > 1e-12) {
            double c2; No* r2;
            separarConstante(no->esquerda, c2, r2);
            c = c2 / den;
            resto = r2;
            return;
        }
    }
    c = 1.0;
    resto = no;
}

// Tabela de antiderivadas básicas: retorna F(u) tal que F'(u) = f(u)
// O chamador aplica a correção da regra da cadeia (dividir por u') quando preciso
No* antiderivadaBasica(const string& nome, No* u) {
    // ∫sin(u)du = -cos(u)
    if (nome == "sin")
        return No::criarOperacao('*', No::criarNumero(-1),
            No::criarFuncao("cos", copiar(u)));
    // ∫cos(u)du = sin(u)
    if (nome == "cos")
        return No::criarFuncao("sin", copiar(u));
    // ∫tan(u)du = -ln(cos(u))
    if (nome == "tan")
        return No::criarOperacao('*', No::criarNumero(-1),
            No::criarFuncao("ln", No::criarFuncao("cos", copiar(u))));
    // ∫cot(u)du = ln(sin(u))
    if (nome == "cot")
        return No::criarFuncao("ln", No::criarFuncao("sin", copiar(u)));
    // ∫sec(u)du = ln(sec(u) + tan(u))
    if (nome == "sec")
        return No::criarFuncao("ln", No::criarOperacao('+',
            No::criarFuncao("sec", copiar(u)), No::criarFuncao("tan", copiar(u))));
    // ∫csc(u)du = -ln(csc(u) + cot(u))
    if (nome == "csc")
        return No::criarOperacao('*', No::criarNumero(-1),
            No::criarFuncao("ln", No::criarOperacao('+',
                No::criarFuncao("csc", copiar(u)), No::criarFuncao("cot", copiar(u)))));
    // ∫e^u du = e^u
    if (nome == "exp")
        return No::criarFuncao("exp", copiar(u));
    // ∫sinh(u)du = cosh(u)
    if (nome == "sinh")
        return No::criarFuncao("cosh", copiar(u));
    // ∫cosh(u)du = sinh(u)
    if (nome == "cosh")
        return No::criarFuncao("sinh", copiar(u));
    // ∫tanh(u)du = ln(cosh(u))
    if (nome == "tanh")
        return No::criarFuncao("ln", No::criarFuncao("cosh", copiar(u)));
    // ∫√u du = (2/3)·√(u³)
    if (nome == "sqrt")
        return No::criarOperacao('/',
            No::criarOperacao('*', No::criarNumero(2),
                No::criarFuncao("sqrt", No::criarOperacao('^', copiar(u), No::criarNumero(3)))),
            No::criarNumero(3));
    // ∫ln(u)du = u·ln(u) - u  (integração por partes)
    if (nome == "ln")
        return No::criarOperacao('-',
            No::criarOperacao('*', copiar(u), No::criarFuncao("ln", copiar(u))),
            copiar(u));
    // ∫log₁₀(u)du = (u·ln(u) - u)/ln(10)
    if (nome == "log")
        return No::criarOperacao('/',
            No::criarOperacao('-',
                No::criarOperacao('*', copiar(u), No::criarFuncao("ln", copiar(u))),
                copiar(u)),
            No::criarFuncao("ln", No::criarNumero(10)));
    // ∫atan(u)du = u·atan(u) - ln(1+u²)/2
    if (nome == "atan")
        return No::criarOperacao('-',
            No::criarOperacao('*', copiar(u), No::criarFuncao("atan", copiar(u))),
            No::criarOperacao('/',
                No::criarFuncao("ln", No::criarOperacao('+', No::criarNumero(1),
                    No::criarOperacao('^', copiar(u), No::criarNumero(2)))),
                No::criarNumero(2)));
    // ∫asin(u)du = u·asin(u) + √(1-u²)
    if (nome == "asin")
        return No::criarOperacao('+',
            No::criarOperacao('*', copiar(u), No::criarFuncao("asin", copiar(u))),
            No::criarFuncao("sqrt", No::criarOperacao('-', No::criarNumero(1),
                No::criarOperacao('^', copiar(u), No::criarNumero(2)))));
    // ∫acos(u)du = u·acos(u) - √(1-u²)
    if (nome == "acos")
        return No::criarOperacao('-',
            No::criarOperacao('*', copiar(u), No::criarFuncao("acos", copiar(u))),
            No::criarFuncao("sqrt", No::criarOperacao('-', No::criarNumero(1),
                No::criarOperacao('^', copiar(u), No::criarNumero(2)))));
    // ∫|u|du = u·|u|/2
    if (nome == "abs")
        return No::criarOperacao('/',
            No::criarOperacao('*', copiar(u), No::criarFuncao("abs", copiar(u))),
            No::criarNumero(2));
    return nullptr; // Função sem antiderivada na tabela
}

// ---- Substituição Simples (u-substituição) ----
// Reconhece o padrão ∫f(u(x))·u'(x)dx em um produto fator1·fator2:
//   Caso A: fator2 = u^n  e fator1 = k·u'  →  k·u^(n+1)/(n+1)
//   Caso B: fator2 = f(u) e fator1 = k·u'  →  k·F(u)
//   Caso C: fator2 = a^u  e fator1 = k·u'  →  k·a^u/ln(a)
// A proporcionalidade entre fator1 e u' é verificada numericamente
No* tentarSubstituicao(No* fator1, No* fator2, char var) {
    if (!fator1 || !fator2) return nullptr;
    double k;

    // ---- Caso A: potência u^n (n constante; fator2 puro equivale a n = 1) ----
    No* base = fator2;
    double n = 1.0;
    if (fator2->tipoNo == No::OP_NO && fator2->operador == '^' && ehConstante(fator2->direita)) {
        base = fator2->esquerda;
        n = avaliarConstanteNo(fator2->direita);
    }
    // sqrt(g) equivale a g^(1/2): desembrulha para a substituição enxergar o
    // argumento interno g como base (ex.: x/√(1-x²) → base 1-x², n = -1/2)
    if (base->tipoNo == No::FUNC_NO && base->funcao == "sqrt" && base->esquerda) {
        base = base->esquerda;
        n = n * 0.5;
    }
    // u precisa depender de x sem ser o próprio x (x^n já tem regra própria)
    if (!isnan(n) && !ehConstante(base) && base->tipoNo != No::VAR_NO) {
        No* du = simplificarVarias(diferenciar(base, var));
        if (razaoConstante(fator1, du, var, k)) {
            // Caso especial n = -1: ∫u'/u dx = ln(u)
            if (abs(n + 1.0) < 1e-10) {
                return No::criarOperacao('*', No::criarNumero(k),
                    No::criarFuncao("ln", copiar(base)));
            }
            // ∫u^n·u' dx = u^(n+1)/(n+1)
            return No::criarOperacao('*', No::criarNumero(k),
                No::criarOperacao('/',
                    No::criarOperacao('^', copiar(base), No::criarNumero(n + 1)),
                    No::criarNumero(n + 1)));
        }
    }

    // ---- Caso B: função composta f(u), com F vindo da tabela ----
    if (fator2->tipoNo == No::FUNC_NO && fator2->esquerda && !ehConstante(fator2->esquerda)) {
        No* du = simplificarVarias(diferenciar(fator2->esquerda, var));
        if (razaoConstante(fator1, du, var, k)) {
            No* F = antiderivadaBasica(fator2->funcao, fator2->esquerda);
            if (F) return No::criarOperacao('*', No::criarNumero(k), F);
        }
    }

    // ---- Caso C: exponencial composta a^u (cobre e^u, que o parser gera como '^') ----
    if (fator2->tipoNo == No::OP_NO && fator2->operador == '^' &&
        ehConstante(fator2->esquerda) && !ehConstante(fator2->direita)) {
        double baseVal = avaliarConstanteNo(fator2->esquerda);
        if (baseVal > 0 && abs(baseVal - 1.0) > 1e-12) {
            No* du = simplificarVarias(diferenciar(fator2->direita, var));
            if (razaoConstante(fator1, du, var, k)) {
                // ∫a^u·u' dx = a^u/ln(a)  (para base e, ln(e) = 1 some na simplificação)
                return No::criarOperacao('*', No::criarNumero(k),
                    No::criarOperacao('/', copiar(fator2),
                        No::criarFuncao("ln", copiar(fator2->esquerda))));
            }
        }
    }

    return nullptr;
}

// ---- Integração por Partes (padrão polinômio × função) ----
// Para p(x) = c·x^n (n inteiro ≥ 1) e g(x) da classe sin/cos/exp/sinh/cosh
// com argumento linear (ou a^(linear)):
//   ∫p·g dx = p·G - ∫p'·G dx     (a recursão reduz o grau de p até 0)
// Também resolve o padrão x^n·ln(x) com fórmula fechada.
No* tentarPorPartes(No* fator1, No* fator2, char var) {
    if (!fator1 || !fator2) return nullptr;

    // fator1 precisa ser um monômio c·x^n com n inteiro ≥ 1
    double coef, grau;
    if (!ehMonomio(fator1, var, coef, grau)) return nullptr;
    if (grau < 1.0 - 1e-10 || abs(grau - round(grau)) > 1e-10) return nullptr;

    // Extrai a constante multiplicativa do segundo fator
    double c; No* resto;
    separarConstante(fator2, c, resto);

    // ---- Padrão x^n·ln(x): ∫ = x^(n+1)·ln(x)/(n+1) - x^(n+1)/(n+1)² ----
    if (resto->tipoNo == No::FUNC_NO && resto->funcao == "ln" &&
        resto->esquerda && resto->esquerda->tipoNo == No::VAR_NO &&
        resto->esquerda->variavel == var) {
        double n1 = grau + 1.0;
        No* resultado = No::criarOperacao('-',
            No::criarOperacao('/',
                No::criarOperacao('*',
                    No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(n1)),
                    No::criarFuncao("ln", No::criarVariavel(var))),
                No::criarNumero(n1)),
            No::criarOperacao('/',
                No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(n1)),
                No::criarNumero(n1 * n1)));
        double k = coef * c;
        if (abs(k - 1.0) < 1e-12) return resultado;
        return No::criarOperacao('*', No::criarNumero(k), resultado);
    }

    // ---- Classe permitida: sin/cos/exp/sinh/cosh(linear) ou a^(linear) ----
    // A antiderivada dessas funções permanece na mesma classe, o que garante
    // que a recursão da integração por partes sempre termina
    double a, b;
    bool classeOk = false;
    if (resto->tipoNo == No::FUNC_NO &&
        (resto->funcao == "sin" || resto->funcao == "cos" || resto->funcao == "exp" ||
         resto->funcao == "sinh" || resto->funcao == "cosh") &&
        ehLinear(resto->esquerda, var, a, b) && abs(a) > 1e-12) {
        classeOk = true;
    }
    if (resto->tipoNo == No::OP_NO && resto->operador == '^' &&
        ehConstante(resto->esquerda) &&
        ehLinear(resto->direita, var, a, b) && abs(a) > 1e-12) {
        classeOk = true;
    }
    if (!classeOk) return nullptr;

    // G = ∫g dx (sempre existe dentro da classe, via regras de argumento linear)
    No* G = integrar(fator2, var);
    if (!G) return nullptr;
    G = simplificarVarias(G);

    // ∫p'·G dx — o grau de p' é menor que o de p, então a recursão termina
    No* dp = simplificarVarias(diferenciar(fator1, var));
    No* integralResto = integrar(No::criarOperacao('*', dp, copiar(G)), var);
    if (!integralResto) return nullptr;

    // ∫p·g dx = p·G - ∫p'·G dx
    return No::criarOperacao('-',
        No::criarOperacao('*', copiar(fator1), G),
        integralResto);
}

// ============================================================
//   APOIO À INTEGRAÇÃO DE FUNÇÕES RACIONAIS (frações parciais)
//   Usa ARRANJOS de coeficientes (coef[i] = coeficiente de x^i),
//   LAÇOS e RECURSÃO — conceitos do programa da disciplina.
// ============================================================

// Grau máximo de polinômio suportado na integração racional
static const int POLI_MAX = 8;

// Monta o nó "a·x + b" (simplifica quando a=1 ou b=0)
No* criarLinear(double a, double b, char var) {
    No* termoX;
    if (abs(a - 1.0) < 1e-12) termoX = No::criarVariavel(var);
    else termoX = No::criarOperacao('*', No::criarNumero(a), No::criarVariavel(var));
    if (abs(b) < 1e-12) return termoX;
    return No::criarOperacao('+', termoX, No::criarNumero(b));
}

// Reconstrói o nó de um polinômio a partir do arranjo de coeficientes
No* polinomioParaNo(const double* coef, int grau, char var) {
    No* resultado = nullptr;
    for (int i = 0; i <= grau; i++) {
        if (abs(coef[i]) < 1e-12) continue;
        No* termo;
        if (i == 0) {
            termo = No::criarNumero(coef[i]);
        } else if (i == 1) {
            termo = No::criarOperacao('*', No::criarNumero(coef[i]), No::criarVariavel(var));
        } else {
            termo = No::criarOperacao('*', No::criarNumero(coef[i]),
                        No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(i)));
        }
        if (!resultado) resultado = termo;
        else resultado = No::criarOperacao('+', resultado, termo);
    }
    if (!resultado) resultado = No::criarNumero(0);
    return resultado;
}

// Extrai os coeficientes de um polinômio em 'var' para o arranjo coef[0..grau].
// Retorna true se a sub-árvore for um polinômio; false caso contrário
// (ex.: contém função, ou divisão por algo não-constante).
bool extrairPolinomio(No* no, char var, double* coef, int& grau) {
    for (int i = 0; i <= POLI_MAX; i++) coef[i] = 0.0; // zera o arranjo
    grau = 0;
    if (!no) return false;

    // Constante (inclui pi, e e sub-árvores numéricas)
    if (ehConstante(no)) {
        double v = avaliarConstanteNo(no);
        if (isnan(v) || isinf(v)) return false;
        coef[0] = v; grau = 0;
        return true;
    }

    // A própria variável de integração: x  ->  coef[1] = 1
    if (no->tipoNo == No::VAR_NO) {
        if (no->variavel != var) return false; // outra variável: não tratamos
        coef[1] = 1.0; grau = 1;
        return true;
    }

    if (no->tipoNo == No::OP_NO) {
        double a[POLI_MAX + 1], b[POLI_MAX + 1];
        int ga, gb;

        // Soma e subtração: somam-se os arranjos termo a termo
        if (no->operador == '+' || no->operador == '-') {
            if (!extrairPolinomio(no->esquerda, var, a, ga)) return false;
            if (!extrairPolinomio(no->direita, var, b, gb)) return false;
            grau = (ga > gb) ? ga : gb;
            for (int i = 0; i <= grau; i++)
                coef[i] = a[i] + (no->operador == '+' ? b[i] : -b[i]);
            while (grau > 0 && abs(coef[grau]) < 1e-12) grau--; // remove zeros do topo
            return true;
        }

        // Multiplicação: convolução dos arranjos de coeficientes
        if (no->operador == '*') {
            if (!extrairPolinomio(no->esquerda, var, a, ga)) return false;
            if (!extrairPolinomio(no->direita, var, b, gb)) return false;
            if (ga + gb > POLI_MAX) return false;
            grau = ga + gb;
            for (int i = 0; i <= ga; i++)
                for (int j = 0; j <= gb; j++)
                    coef[i + j] += a[i] * b[j];
            while (grau > 0 && abs(coef[grau]) < 1e-12) grau--;
            return true;
        }

        // Divisão: polinômio apenas se o denominador for constante
        if (no->operador == '/') {
            if (!ehConstante(no->direita)) return false;
            double d = avaliarConstanteNo(no->direita);
            if (abs(d) < 1e-12) return false;
            if (!extrairPolinomio(no->esquerda, var, a, ga)) return false;
            grau = ga;
            for (int i = 0; i <= ga; i++) coef[i] = a[i] / d;
            return true;
        }

        // Potência: base polinômio, expoente inteiro constante >= 0
        if (no->operador == '^') {
            if (!ehConstante(no->direita)) return false;
            double e = avaliarConstanteNo(no->direita);
            int n = (int)round(e);
            if (abs(e - n) > 1e-9 || n < 0) return false;
            if (!extrairPolinomio(no->esquerda, var, a, ga)) return false;
            if (ga * n > POLI_MAX) return false;
            // r = a^n via multiplicação repetida (começa em 1)
            double r[POLI_MAX + 1];
            for (int i = 0; i <= POLI_MAX; i++) r[i] = 0.0;
            r[0] = 1.0;
            int gr = 0;
            for (int p = 0; p < n; p++) {
                double tmp[POLI_MAX + 1];
                for (int i = 0; i <= POLI_MAX; i++) tmp[i] = 0.0;
                for (int i = 0; i <= gr; i++)
                    for (int j = 0; j <= ga; j++)
                        tmp[i + j] += r[i] * a[j];
                for (int i = 0; i <= POLI_MAX; i++) r[i] = tmp[i];
                gr += ga;
            }
            grau = gr;
            for (int i = 0; i <= gr; i++) coef[i] = r[i];
            while (grau > 0 && abs(coef[grau]) < 1e-12) grau--;
            return true;
        }
    }

    return false; // FUNC_NO ou caso não reconhecido: não é polinômio
}

// Divisão de polinômios: num / den = quo, resto = rem (índices = potências)
void dividirPolinomio(const double* num, int gn, const double* den, int gd,
                      double* quo, int& gq, double* rem, int& gr) {
    double r[POLI_MAX + 1];
    for (int i = 0; i <= POLI_MAX; i++) { r[i] = (i <= gn) ? num[i] : 0.0; quo[i] = 0.0; }

    for (int k = gn - gd; k >= 0; k--) {
        double fator = r[k + gd] / den[gd];
        quo[k] = fator;
        for (int j = 0; j <= gd; j++) r[k + j] -= fator * den[j];
    }
    gq = (gn >= gd) ? gn - gd : 0;
    while (gq > 0 && abs(quo[gq]) < 1e-12) gq--;

    for (int i = 0; i <= POLI_MAX; i++) rem[i] = (i < gd) ? r[i] : 0.0;
    gr = 0;
    for (int i = gd - 1; i > 0; i--) { if (abs(rem[i]) > 1e-12) { gr = i; break; } }
}

// Integra uma função racional P(x)/Q(x) por divisão polinomial + frações parciais.
// Cobre denominador de grau 1 e 2 (raízes reais distintas, raiz dupla, ou
// raízes complexas via completar o quadrado). Retorna nullptr se não se aplica.
No* integrarRacional(No* num, No* den, char var) {
    double cn[POLI_MAX + 1], cd[POLI_MAX + 1];
    int gn, gd;
    if (!extrairPolinomio(num, var, cn, gn)) return nullptr;
    if (!extrairPolinomio(den, var, cd, gd)) return nullptr;
    if (gd < 1 || gd > 2) return nullptr; // só linear e quadrático

    // ---- Parte inteira: se grau(num) >= grau(den), faz a divisão ----
    No* parteInteira = nullptr;
    double cnum[POLI_MAX + 1];
    int gnum;
    for (int i = 0; i <= POLI_MAX; i++) cnum[i] = cn[i];
    gnum = gn;

    if (gn >= gd) {
        double quo[POLI_MAX + 1], rem[POLI_MAX + 1];
        int gq, gr;
        dividirPolinomio(cn, gn, cd, gd, quo, gq, rem, gr);
        parteInteira = integrar(polinomioParaNo(quo, gq, var), var);
        if (!parteInteira) return nullptr;
        for (int i = 0; i <= POLI_MAX; i++) cnum[i] = rem[i];
        gnum = gr;
        bool restoZero = true;
        for (int i = 0; i <= gnum; i++) if (abs(cnum[i]) > 1e-12) restoZero = false;
        if (restoZero) return parteInteira;
    }

    // ---- Fração própria: numerador (p·x + q) sobre o denominador ----
    No* parteFrac = nullptr;
    double p = (gnum >= 1) ? cnum[1] : 0.0;
    double q = cnum[0];

    if (gd == 1) {
        // q / (a·x + b)  ->  (q/a)·ln(a·x + b)
        parteFrac = No::criarOperacao('*',
            No::criarNumero(q / cd[1]),
            No::criarFuncao("ln", criarLinear(cd[1], cd[0], var)));
    } else {
        // A·x² + B·x + C
        double A = cd[2], B = cd[1], C = cd[0];
        double disc = B * B - 4 * A * C;

        if (disc > 1e-9) {
            // Duas raízes reais distintas: frações parciais
            double sq = sqrt(disc);
            double r1 = (-B + sq) / (2 * A);
            double r2 = (-B - sq) / (2 * A);
            double K1 = (p * r1 + q) / (A * (r1 - r2));
            double K2 = (p * r2 + q) / (A * (r2 - r1));
            parteFrac = No::criarOperacao('+',
                No::criarOperacao('*', No::criarNumero(K1),
                    No::criarFuncao("ln", criarLinear(1.0, -r1, var))),
                No::criarOperacao('*', No::criarNumero(K2),
                    No::criarFuncao("ln", criarLinear(1.0, -r2, var))));
        } else if (disc < -1e-9) {
            // Raízes complexas: parte ln (de u'/u) + parte atan (completar quadrado)
            No* termoLn = nullptr;
            if (abs(p) > 1e-12) {
                termoLn = No::criarOperacao('*', No::criarNumero(p / (2 * A)),
                            No::criarFuncao("ln", copiar(den)));
            }
            double resto = q - p * B / (2 * A);
            No* termoAtan = nullptr;
            if (abs(resto) > 1e-12) {
                // A·x²+B·x+C = A·[(x + B/2A)² + k], com k = (4AC - B²)/(4A²) > 0
                double s = B / (2 * A);
                double k = (4 * A * C - B * B) / (4 * A * A);
                double raizk = sqrt(k);
                No* arg = No::criarOperacao('/', criarLinear(1.0, s, var), No::criarNumero(raizk));
                termoAtan = No::criarOperacao('*',
                    No::criarNumero(resto / (A * raizk)),
                    No::criarFuncao("atan", arg));
            }
            if (termoLn && termoAtan) parteFrac = No::criarOperacao('+', termoLn, termoAtan);
            else parteFrac = termoLn ? termoLn : termoAtan;
            if (!parteFrac) parteFrac = No::criarNumero(0);
        } else {
            // Raiz dupla r: (p·x+q)/(A(x-r)²) = (p/A)·ln|x-r| - (q+p·r)/(A(x-r))
            double r = -B / (2 * A);
            parteFrac = No::criarOperacao('+',
                No::criarOperacao('*', No::criarNumero(p / A),
                    No::criarFuncao("ln", criarLinear(1.0, -r, var))),
                No::criarOperacao('/', No::criarNumero(-(q + p * r) / A),
                    criarLinear(1.0, -r, var)));
        }
    }

    if (!parteFrac) return parteInteira;
    if (parteInteira) return No::criarOperacao('+', parteInteira, parteFrac);
    return parteFrac;
}

// Verifica se o nó é exponencial de argumento linear: e^(a·x+c) ou exp(a·x+c).
// Preenche 'taxa' (= a) e devolve o próprio nó exponencial em 'expNode'.
bool ehExpLinear(No* no, char var, double& taxa, No*& expNode) {
    double a, b;
    if (no->tipoNo == No::FUNC_NO && no->funcao == "exp" &&
        ehLinear(no->esquerda, var, a, b)) {
        taxa = a; expNode = no; return abs(a) > 1e-12;
    }
    if (no->tipoNo == No::OP_NO && no->operador == '^' && ehConstante(no->esquerda) &&
        abs(avaliarConstanteNo(no->esquerda) - E_VALOR) < 1e-6 &&
        ehLinear(no->direita, var, a, b)) {
        taxa = a; expNode = no; return abs(a) > 1e-12;
    }
    return false;
}

// Integração por partes CÍCLICA: ∫e^(a·x)·sin(b·x) dx e ∫e^(a·x)·cos(b·x) dx.
//   ∫e^p·sin(q) dx = e^p·(a·sin(q) - b·cos(q)) / (a²+b²)
//   ∫e^p·cos(q) dx = e^p·(a·cos(q) + b·sin(q)) / (a²+b²)
No* tentarPartesCiclica(No* fator1, No* fator2, char var) {
    if (!fator1 || !fator2) return nullptr;
    double a;
    No* expNode = nullptr;
    No* trig = nullptr;
    if (ehExpLinear(fator1, var, a, expNode)) trig = fator2;
    else if (ehExpLinear(fator2, var, a, expNode)) trig = fator1;
    else return nullptr;

    if (trig->tipoNo != No::FUNC_NO) return nullptr;
    if (trig->funcao != "sin" && trig->funcao != "cos") return nullptr;
    double b, d;
    if (!ehLinear(trig->esquerda, var, b, d) || abs(b) < 1e-12) return nullptr;

    No* arg = trig->esquerda;
    No* combinacao;
    if (trig->funcao == "sin") {
        combinacao = No::criarOperacao('-',
            No::criarOperacao('*', No::criarNumero(a), No::criarFuncao("sin", copiar(arg))),
            No::criarOperacao('*', No::criarNumero(b), No::criarFuncao("cos", copiar(arg))));
    } else {
        combinacao = No::criarOperacao('+',
            No::criarOperacao('*', No::criarNumero(a), No::criarFuncao("cos", copiar(arg))),
            No::criarOperacao('*', No::criarNumero(b), No::criarFuncao("sin", copiar(arg))));
    }
    return No::criarOperacao('/',
        No::criarOperacao('*', copiar(expNode), combinacao),
        No::criarNumero(a * a + b * b));
}

// Integração por partes com FUNÇÃO INVERSA: ∫x·atan(x), ∫x·asin(x), ∫x·acos(x).
// Usa fórmulas fechadas (deriva a inversa e integra o x). fator1 = c·x, arg = x.
No* tentarPartesInversa(No* fator1, No* fator2, char var) {
    if (!fator1 || !fator2) return nullptr;
    double coef, grau;
    if (!ehMonomio(fator1, var, coef, grau)) return nullptr;
    if (abs(grau - 1.0) > 1e-10) return nullptr;           // apenas grau 1 (x)
    if (fator2->tipoNo != No::FUNC_NO) return nullptr;
    No* u = fator2->esquerda;
    if (!u || u->tipoNo != No::VAR_NO || u->variavel != var) return nullptr; // argumento = x
    string fn = fator2->funcao;

    No* corpo = nullptr;
    if (fn == "atan") {
        // ((x²+1)·atan(x) - x) / 2
        corpo = No::criarOperacao('/',
            No::criarOperacao('-',
                No::criarOperacao('*',
                    No::criarOperacao('+',
                        No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(2)),
                        No::criarNumero(1)),
                    No::criarFuncao("atan", No::criarVariavel(var))),
                No::criarVariavel(var)),
            No::criarNumero(2));
    } else if (fn == "asin" || fn == "acos") {
        // (2x²-1) e √(1-x²)
        No* doisX2menos1 = No::criarOperacao('-',
            No::criarOperacao('*', No::criarNumero(2),
                No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(2))),
            No::criarNumero(1));
        No* raiz = No::criarFuncao("sqrt",
            No::criarOperacao('-', No::criarNumero(1),
                No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(2))));
        No* termoRaiz = No::criarOperacao('*', No::criarVariavel(var), raiz);
        // asin: ((2x²-1)·asin(x) + x√(1-x²))/4 ;  acos: ((2x²-1)·acos(x) - x√(1-x²))/4
        No* termoFn = No::criarOperacao('*', doisX2menos1,
            No::criarFuncao(fn, No::criarVariavel(var)));
        No* soma = (fn == "asin")
            ? No::criarOperacao('+', termoFn, termoRaiz)
            : No::criarOperacao('-', termoFn, termoRaiz);
        corpo = No::criarOperacao('/', soma, No::criarNumero(4));
    } else {
        return nullptr;
    }

    if (abs(coef - 1.0) < 1e-12) return corpo;
    return No::criarOperacao('*', No::criarNumero(coef), corpo);
}

// ---- Integração Simbólica ----
// Calcula ∫f(x)dx aplicando regras de integração sobre a árvore:
// regras imediatas, substituição linear ∫f(ax+b)dx = F(ax+b)/a,
// substituição simples ∫f(u)·u'dx = F(u) e integração por partes
// Retorna nullptr quando a expressão não pode ser integrada simbolicamente
// Nesse caso, o programa usa o método numérico (Simpson) como fallback
No* integrar(No* no, char var) {
    if (!no) return nullptr;

    switch (no->tipoNo) {

        // ∫c dx = c·x  — integral de uma constante
        case No::NUM_NO:
            return No::criarOperacao('*', copiar(no), No::criarVariavel(var));

        // ∫x dx = x²/2  — integral de variável simples
        case No::VAR_NO:
            if (no->variavel == var) {
                return No::criarOperacao('/',
                    No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(2)),
                    No::criarNumero(2));
            }
            // ∫y dx = y·x  — variável diferente é constante
            return No::criarOperacao('*', copiar(no), No::criarVariavel(var));

        // Regras para operadores
        case No::OP_NO: {
            switch (no->operador) {

                // ∫(f + g)dx = ∫f dx + ∫g dx  — linearidade da integral (soma)
                case '+': {
                    No* intF = integrar(no->esquerda, var);
                    No* intG = integrar(no->direita, var);
                    if (!intF || !intG) return nullptr; // Fallback se algum termo falhar
                    return No::criarOperacao('+', intF, intG);
                }

                // ∫(f - g)dx = ∫f dx - ∫g dx  — linearidade da integral (diferença)
                case '-': {
                    No* intF = integrar(no->esquerda, var);
                    No* intG = integrar(no->direita, var);
                    if (!intF || !intG) return nullptr;
                    return No::criarOperacao('-', intF, intG);
                }

                // ∫(c · f)dx = c · ∫f dx  — fator constante pode sair da integral
                case '*': {
                    if (ehConstante(no->esquerda)) {
                        No* intG = integrar(no->direita, var);
                        if (!intG) return nullptr;
                        return No::criarOperacao('*', copiar(no->esquerda), intG);
                    }
                    if (ehConstante(no->direita)) {
                        No* intF = integrar(no->esquerda, var);
                        if (!intF) return nullptr;
                        return No::criarOperacao('*', intF, copiar(no->direita));
                    }

                    // ---- Monômios: x·x, x²·3x, ... → ∫c·x^n dx direto ----
                    double coefM, grauM;
                    if (ehMonomio(no, var, coefM, grauM)) {
                        return integrarMonomio(coefM, grauM, var);
                    }

                    // ---- Substituição simples: padrão f(u)·u' (testa as duas ordens) ----
                    // Ex.: sin(x)·cos(x) → sin²(x)/2,  x·exp(x²) → exp(x²)/2
                    No* porSubst = tentarSubstituicao(no->esquerda, no->direita, var);
                    if (!porSubst) porSubst = tentarSubstituicao(no->direita, no->esquerda, var);
                    if (porSubst) return porSubst;

                    // ---- Integração por partes: padrão x^n·(sin|cos|exp|ln|...) ----
                    // Ex.: x·sin(x) → sin(x) - x·cos(x),  x·ln(x) → x²·ln(x)/2 - x²/4
                    No* porPartes = tentarPorPartes(no->esquerda, no->direita, var);
                    if (!porPartes) porPartes = tentarPorPartes(no->direita, no->esquerda, var);
                    if (porPartes) return porPartes;

                    // ---- Por partes cíclica: e^(a·x)·sin(b·x) / e^(a·x)·cos(b·x) ----
                    // Ex.: e^x·sin(x) → e^x·(sin(x) - cos(x))/2
                    No* ciclica = tentarPartesCiclica(no->esquerda, no->direita, var);
                    if (ciclica) return ciclica;

                    // ---- Por partes com função inversa: x·atan(x), x·asin(x), x·acos(x) ----
                    No* inversa = tentarPartesInversa(no->esquerda, no->direita, var);
                    if (!inversa) inversa = tentarPartesInversa(no->direita, no->esquerda, var);
                    if (inversa) return inversa;

                    return nullptr; // Produto sem padrão reconhecido — fallback numérico
                }

                // ∫(f / c)dx = (1/c) · ∫f dx  — divisor constante pode sair da integral
                // ∫(c / x)dx = c · ln(x)  — caso especial
                case '/': {
                    if (ehConstante(no->direita)) {
                        No* intF = integrar(no->esquerda, var);
                        if (!intF) return nullptr;
                        return No::criarOperacao('/', intF, copiar(no->direita));
                    }
                    // c/x → c·ln(x)
                    if (ehConstante(no->esquerda) &&
                        no->direita->tipoNo == No::VAR_NO && no->direita->variavel == var) {
                        return No::criarOperacao('*', copiar(no->esquerda),
                            No::criarFuncao("ln", No::criarVariavel(var)));
                    }

                    // ---- Monômios: x²/x, 1/x³, ... ----
                    double coefM, grauM, aLin, bLin;
                    if (ehMonomio(no, var, coefM, grauM)) {
                        return integrarMonomio(coefM, grauM, var);
                    }

                    // ---- c/(a·x+b) → (c/a)·ln(a·x+b)  — substituição linear ----
                    if (ehConstante(no->esquerda) &&
                        ehLinear(no->direita, var, aLin, bLin) && abs(aLin) > 1e-12) {
                        return No::criarOperacao('*',
                            No::criarOperacao('/', copiar(no->esquerda), No::criarNumero(aLin)),
                            No::criarFuncao("ln", copiar(no->direita)));
                    }

                    // ---- c/(b + u²) com b > 0 e u linear → (c/(a·√b))·atan(u/√b) ----
                    // Generaliza o caso 1/(1+x²)=atan(x); ex.: 1/(4+x²) = ½·atan(x/2)
                    if (ehConstante(no->esquerda) &&
                        no->direita->tipoNo == No::OP_NO && no->direita->operador == '+') {
                        No* termoConst = nullptr; // o termo b (constante)
                        No* quadrado = nullptr;    // o termo u²
                        if (ehConstante(no->direita->esquerda)) {
                            termoConst = no->direita->esquerda; quadrado = no->direita->direita;
                        } else if (ehConstante(no->direita->direita)) {
                            termoConst = no->direita->direita; quadrado = no->direita->esquerda;
                        }

                        if (termoConst && quadrado &&
                            quadrado->tipoNo == No::OP_NO && quadrado->operador == '^' &&
                            quadrado->direita->tipoNo == No::NUM_NO &&
                            abs(quadrado->direita->valor - 2.0) < 1e-10 &&
                            ehLinear(quadrado->esquerda, var, aLin, bLin) && abs(aLin) > 1e-12) {

                            double b = avaliarConstanteNo(termoConst);
                            if (!isnan(b) && b > 1e-12) {
                                double raizB = sqrt(b);
                                double c = avaliarConstanteNo(no->esquerda);
                                // Argumento u/√b (omite a divisão quando √b ≈ 1)
                                No* arg = (abs(raizB - 1.0) < 1e-10)
                                    ? copiar(quadrado->esquerda)
                                    : No::criarOperacao('/', copiar(quadrado->esquerda),
                                                        No::criarNumero(raizB));
                                return No::criarOperacao('*',
                                    No::criarNumero(c / (aLin * raizB)),
                                    No::criarFuncao("atan", arg));
                            }
                        }
                    }

                    // ---- c/√(b - u²) com b > 0 e u linear → (c/(a·√b))·asin(u/√b) ----
                    // Generaliza o caso 1/√(1-x²)=asin(x); ex.: 1/√(9-x²) = asin(x/3)
                    if (ehConstante(no->esquerda) &&
                        no->direita->tipoNo == No::FUNC_NO && no->direita->funcao == "sqrt" &&
                        no->direita->esquerda && no->direita->esquerda->tipoNo == No::OP_NO &&
                        no->direita->esquerda->operador == '-' &&
                        ehConstante(no->direita->esquerda->esquerda)) {
                        No* quadrado = no->direita->esquerda->direita;
                        if (quadrado && quadrado->tipoNo == No::OP_NO && quadrado->operador == '^' &&
                            quadrado->direita->tipoNo == No::NUM_NO &&
                            abs(quadrado->direita->valor - 2.0) < 1e-10 &&
                            ehLinear(quadrado->esquerda, var, aLin, bLin) && abs(aLin) > 1e-12) {

                            double b = avaliarConstanteNo(no->direita->esquerda->esquerda);
                            if (!isnan(b) && b > 1e-12) {
                                double raizB = sqrt(b);
                                double c = avaliarConstanteNo(no->esquerda);
                                // Argumento u/√b (omite a divisão quando √b ≈ 1)
                                No* arg = (abs(raizB - 1.0) < 1e-10)
                                    ? copiar(quadrado->esquerda)
                                    : No::criarOperacao('/', copiar(quadrado->esquerda),
                                                        No::criarNumero(raizB));
                                // Coeficiente é c/a (o √b cancela em ∫dx/√(b-x²)=asin(x/√b))
                                return No::criarOperacao('*',
                                    No::criarNumero(c / aLin),
                                    No::criarFuncao("asin", arg));
                            }
                        }
                    }

                    // ---- Regra u'/u: numerador múltiplo da derivada do denominador ----
                    // Ex.: 2x/(x²+1) → ln(x²+1),  cos(x)/sin(x) → ln(sin(x))
                    {
                        No* du = simplificarVarias(diferenciar(no->direita, var));
                        double k;
                        if (razaoConstante(no->esquerda, du, var, k)) {
                            return No::criarOperacao('*', No::criarNumero(k),
                                No::criarFuncao("ln", copiar(no->direita)));
                        }
                    }

                    // ---- Funções racionais P(x)/Q(x): divisão polinomial + frações parciais ----
                    // Ex.: 1/(x²-1) → ½ln(x-1) - ½ln(x+1),  x²/(x²+1) → x - atan(x)
                    No* racional = integrarRacional(no->esquerda, no->direita, var);
                    if (racional) return racional;

                    // ---- Último recurso: trata f/g como f·g⁻¹ e tenta substituição ----
                    // Ex.: ln(x)/x → ln²(x)/2,  cos(x)/sin²(x) → -1/sin(x)
                    {
                        No* inverso;
                        if (no->direita->tipoNo == No::OP_NO && no->direita->operador == '^' &&
                            ehConstante(no->direita->direita)) {
                            // f/g^n → f·g^(-n)
                            double n = avaliarConstanteNo(no->direita->direita);
                            inverso = No::criarOperacao('^', copiar(no->direita->esquerda),
                                No::criarNumero(-n));
                        } else {
                            inverso = No::criarOperacao('^', copiar(no->direita), No::criarNumero(-1));
                        }
                        No* porSubst = tentarSubstituicao(no->esquerda, inverso, var);
                        if (!porSubst) porSubst = tentarSubstituicao(inverso, no->esquerda, var);
                        if (porSubst) return porSubst;
                    }

                    return nullptr; // Quociente sem padrão reconhecido — fallback numérico
                }

                // ∫x^n dx = x^(n+1)/(n+1)  — regra da potência
                case '^': {
                    // Base = x, expoente = constante n
                    if (no->esquerda->tipoNo == No::VAR_NO && no->esquerda->variavel == var &&
                        ehConstante(no->direita)) {
                        double n = avaliarConstanteNo(no->direita);
                        // Caso especial: ∫x^(-1) dx = ln(x)
                        if (abs(n + 1) < 1e-10) {
                            return No::criarFuncao("ln", No::criarVariavel(var));
                        }
                        // Caso geral: x^(n+1) / (n+1)
                        return No::criarOperacao('/',
                            No::criarOperacao('^', No::criarVariavel(var), No::criarNumero(n + 1)),
                            No::criarNumero(n + 1));
                    }
                    // Base = constante e (Euler), expoente = x → ∫e^x dx = e^x
                    if (ehConstante(no->esquerda) &&
                        abs(avaliarConstanteNo(no->esquerda) - E_VALOR) < 1e-6 &&
                        no->direita->tipoNo == No::VAR_NO && no->direita->variavel == var) {
                        return No::criarOperacao('^',
                            copiar(no->esquerda), No::criarVariavel(var));
                    }
                    // Base = constante a, expoente = x → ∫a^x dx = a^x / ln(a)
                    if (ehConstante(no->esquerda) &&
                        no->direita->tipoNo == No::VAR_NO && no->direita->variavel == var) {
                        return No::criarOperacao('/',
                            No::criarOperacao('^', copiar(no->esquerda), No::criarVariavel(var)),
                            No::criarFuncao("ln", copiar(no->esquerda)));
                    }

                    // ---- (a·x+b)^n → (a·x+b)^(n+1)/(a·(n+1))  — substituição linear ----
                    double aLin, bLin;
                    if (ehConstante(no->direita) &&
                        ehLinear(no->esquerda, var, aLin, bLin) && abs(aLin) > 1e-12) {
                        double n = avaliarConstanteNo(no->direita);
                        if (!isnan(n)) {
                            // Caso especial: ∫(a·x+b)⁻¹ dx = ln(a·x+b)/a
                            if (abs(n + 1.0) < 1e-10) {
                                return No::criarOperacao('/',
                                    No::criarFuncao("ln", copiar(no->esquerda)),
                                    No::criarNumero(aLin));
                            }
                            return No::criarOperacao('/',
                                No::criarOperacao('^', copiar(no->esquerda), No::criarNumero(n + 1)),
                                No::criarNumero(aLin * (n + 1)));
                        }
                    }

                    // ---- a^(k·x+c) → a^(k·x+c)/(k·ln(a))  — exponencial com expoente linear ----
                    if (ehConstante(no->esquerda) &&
                        ehLinear(no->direita, var, aLin, bLin) && abs(aLin) > 1e-12) {
                        double baseVal = avaliarConstanteNo(no->esquerda);
                        if (baseVal > 0 && abs(baseVal - 1.0) > 1e-12) {
                            // Para base e, ln(e) = 1 desaparece na simplificação
                            return No::criarOperacao('/', copiar(no),
                                No::criarOperacao('*', No::criarNumero(aLin),
                                    No::criarFuncao("ln", copiar(no->esquerda))));
                        }
                    }

                    // ---- Quadrados de trigonométricas/hiperbólicas com argumento linear ----
                    // Usa identidades de redução de potência (ex.: sin²u = (1-cos(2u))/2)
                    if (no->esquerda->tipoNo == No::FUNC_NO &&
                        no->direita->tipoNo == No::NUM_NO &&
                        abs(no->direita->valor - 2.0) < 1e-10 &&
                        ehLinear(no->esquerda->esquerda, var, aLin, bLin) && abs(aLin) > 1e-12) {

                        string nome = no->esquerda->funcao;
                        No* u = no->esquerda->esquerda;

                        // ∫sin²(u)dx = x/2 - sin(2u)/(4a)
                        if (nome == "sin")
                            return No::criarOperacao('-',
                                No::criarOperacao('/', No::criarVariavel(var), No::criarNumero(2)),
                                No::criarOperacao('/',
                                    No::criarFuncao("sin",
                                        No::criarOperacao('*', No::criarNumero(2), copiar(u))),
                                    No::criarNumero(4 * aLin)));
                        // ∫cos²(u)dx = x/2 + sin(2u)/(4a)
                        if (nome == "cos")
                            return No::criarOperacao('+',
                                No::criarOperacao('/', No::criarVariavel(var), No::criarNumero(2)),
                                No::criarOperacao('/',
                                    No::criarFuncao("sin",
                                        No::criarOperacao('*', No::criarNumero(2), copiar(u))),
                                    No::criarNumero(4 * aLin)));
                        // ∫tan²(u)dx = tan(u)/a - x
                        if (nome == "tan")
                            return No::criarOperacao('-',
                                No::criarOperacao('/', No::criarFuncao("tan", copiar(u)),
                                    No::criarNumero(aLin)),
                                No::criarVariavel(var));
                        // ∫cot²(u)dx = -cot(u)/a - x
                        if (nome == "cot")
                            return No::criarOperacao('-',
                                No::criarOperacao('*', No::criarNumero(-1),
                                    No::criarOperacao('/', No::criarFuncao("cot", copiar(u)),
                                        No::criarNumero(aLin))),
                                No::criarVariavel(var));
                        // ∫sec²(u)dx = tan(u)/a
                        if (nome == "sec")
                            return No::criarOperacao('/', No::criarFuncao("tan", copiar(u)),
                                No::criarNumero(aLin));
                        // ∫csc²(u)dx = -cot(u)/a
                        if (nome == "csc")
                            return No::criarOperacao('*', No::criarNumero(-1),
                                No::criarOperacao('/', No::criarFuncao("cot", copiar(u)),
                                    No::criarNumero(aLin)));
                        // ∫sinh²(u)dx = sinh(2u)/(4a) - x/2
                        if (nome == "sinh")
                            return No::criarOperacao('-',
                                No::criarOperacao('/',
                                    No::criarFuncao("sinh",
                                        No::criarOperacao('*', No::criarNumero(2), copiar(u))),
                                    No::criarNumero(4 * aLin)),
                                No::criarOperacao('/', No::criarVariavel(var), No::criarNumero(2)));
                        // ∫cosh²(u)dx = sinh(2u)/(4a) + x/2
                        if (nome == "cosh")
                            return No::criarOperacao('+',
                                No::criarOperacao('/',
                                    No::criarFuncao("sinh",
                                        No::criarOperacao('*', No::criarNumero(2), copiar(u))),
                                    No::criarNumero(4 * aLin)),
                                No::criarOperacao('/', No::criarVariavel(var), No::criarNumero(2)));
                        // ∫tanh²(u)dx = x - tanh(u)/a
                        if (nome == "tanh")
                            return No::criarOperacao('-',
                                No::criarVariavel(var),
                                No::criarOperacao('/', No::criarFuncao("tanh", copiar(u)),
                                    No::criarNumero(aLin)));
                    }

                    return nullptr;
                }
            }
            break;
        }

        // Integrais de funções: tabela de antiderivadas + substituição linear
        case No::FUNC_NO: {
            No* u = no->esquerda;
            if (!u) return nullptr;

            // Argumento é a própria variável: consulta direta à tabela
            // (sin, cos, tan, sec, csc, cot, exp, ln, log, sqrt, abs,
            //  sinh, cosh, tanh, asin, acos, atan)
            if (u->tipoNo == No::VAR_NO && u->variavel == var) {
                return antiderivadaBasica(no->funcao, u);
            }

            // ---- Substituição linear: ∫f(a·x+b)dx = F(a·x+b)/a ----
            // Ex.: ∫sin(2x)dx = -cos(2x)/2,  ∫e^(-x)dx = -e^(-x)
            double aLin, bLin;
            if (ehLinear(u, var, aLin, bLin) && abs(aLin) > 1e-12) {
                No* F = antiderivadaBasica(no->funcao, u);
                if (F) return No::criarOperacao('/', F, No::criarNumero(aLin));
            }

            return nullptr; // Argumento complexo — sem regra direta
        }
    }

    return nullptr;
}

// Verifica se dois nós são estruturalmente idênticos (para simplificação)
bool saoIguais(No* a, No* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a->tipoNo != b->tipoNo) return false;
    if (a->tipoNo == No::NUM_NO) return abs(a->valor - b->valor) < 1e-10;
    if (a->tipoNo == No::VAR_NO) return a->variavel == b->variavel;
    if (a->tipoNo == No::OP_NO) return a->operador == b->operador && saoIguais(a->esquerda, b->esquerda) && saoIguais(a->direita, b->direita);
    if (a->tipoNo == No::FUNC_NO) return a->funcao == b->funcao && saoIguais(a->esquerda, b->esquerda);
    return false;
}

// ---- Simplificação Algébrica ----
// Reduz a expressão aplicando identidades aritméticas e folding de constantes
// Exemplo: 2 * x^(2 - 1) * 1  →  2*x
No* simplificar(No* no) {
    if (!no) return nullptr;

    // Primeiro, simplifica recursivamente os filhos
    if (no->esquerda) no->esquerda = simplificar(no->esquerda);
    if (no->direita)  no->direita  = simplificar(no->direita);

    // Folding de constantes: se toda a sub-árvore é constante, calcula o valor
    if (no->tipoNo != No::NUM_NO && ehConstante(no)) {
        double val = avaliarConstanteNo(no);
        if (!isnan(val) && !isinf(val)) {
            return No::criarNumero(val);       // Substitui sub-árvore por um único número
        }
    }

    // Identidades aritméticas para operadores
    if (no->tipoNo == No::OP_NO) {
        switch (no->operador) {

            case '+':
                if (saoIguais(no->esquerda, no->direita)) return No::criarOperacao('*', No::criarNumero(2), no->esquerda);
                if (ehZero(no->esquerda)) return no->direita;  // 0 + g = g
                if (ehZero(no->direita))  return no->esquerda; // f + 0 = f

                // ---- Somar constantes soltas: (f + c1) + c2  -->  f + (c1+c2) ----
                if (no->esquerda->tipoNo == No::OP_NO && no->esquerda->operador == '+' &&
                    no->esquerda->direita->tipoNo == No::NUM_NO &&
                    no->direita->tipoNo == No::NUM_NO) {

                    double c1 = no->esquerda->direita->valor;
                    double c2 = no->direita->valor;

                    return No::criarOperacao('+', copiar(no->esquerda->esquerda), No::criarNumero(c1 + c2));
                }

                // ---- f + (-c)·g  -->  f - c·g — soma com negativo vira subtração ----
                if (no->direita->tipoNo == No::OP_NO && no->direita->operador == '*' &&
                    no->direita->esquerda->tipoNo == No::NUM_NO &&
                    no->direita->esquerda->valor < 0) {
                    return No::criarOperacao('-', no->esquerda,
                        No::criarOperacao('*', No::criarNumero(-no->direita->esquerda->valor),
                            no->direita->direita));
                }

                break;

            case '-':
                if (saoIguais(no->esquerda, no->direita)) return No::criarNumero(0);
                if (ehZero(no->direita))  return no->esquerda; // f - 0 = f
                if (ehZero(no->esquerda)) {                     // 0 - g = -g
                    return No::criarOperacao('*', No::criarNumero(-1), no->direita);
                }

                // ---- Somar constantes soltas: (f - c1) - c2  -->  f - (c1+c2) ----
                if (no->esquerda->tipoNo == No::OP_NO && no->esquerda->operador == '-' &&
                    no->esquerda->direita->tipoNo == No::NUM_NO &&
                    no->direita->tipoNo == No::NUM_NO && no->operador == '-') {

                    double c1 = no->esquerda->direita->valor;
                    double c2 = no->direita->valor;

                    return No::criarOperacao('-', copiar(no->esquerda->esquerda), No::criarNumero(c1 + c2));
                }

                // ---- f - (-c)·g  -->  f + c·g — remove a dupla negação da subtração ----
                if (no->direita->tipoNo == No::OP_NO && no->direita->operador == '*' &&
                    no->direita->esquerda->tipoNo == No::NUM_NO &&
                    no->direita->esquerda->valor < 0) {
                    return No::criarOperacao('+', no->esquerda,
                        No::criarOperacao('*', No::criarNumero(-no->direita->esquerda->valor),
                            no->direita->direita));
                }

                break;

            case '*':
                if (saoIguais(no->esquerda, no->direita)) return No::criarOperacao('^', no->esquerda, No::criarNumero(2));
                if (ehZero(no->esquerda) || ehZero(no->direita)) // f * 0 = 0 ou 0 * g = 0
                    return No::criarNumero(0);
                if (ehUm(no->esquerda)) return no->direita;      // 1 * g = g
                if (ehUm(no->direita))  return no->esquerda;     // f * 1 = f
                // (-1) * ((-1) * g) = g — dupla negação
                if (no->esquerda->tipoNo == No::NUM_NO && no->esquerda->valor == -1 &&
                    no->direita->tipoNo == No::OP_NO && no->direita->operador == '*' &&
                    no->direita->esquerda->tipoNo == No::NUM_NO && no->direita->esquerda->valor == -1) {
                    return no->direita->direita;
                }
                // ---- c1 * (c2 * g)  -->  (c1·c2) * g — agrupa constantes aninhadas ----
                if (no->esquerda->tipoNo == No::NUM_NO &&
                    no->direita->tipoNo == No::OP_NO && no->direita->operador == '*' &&
                    no->direita->esquerda->tipoNo == No::NUM_NO) {
                    return No::criarOperacao('*',
                        No::criarNumero(no->esquerda->valor * no->direita->esquerda->valor),
                        no->direita->direita);
                }
                // ---- c1 * (f / c2)  -->  (c1/c2) * f — corta a fração ----
                if (no->esquerda->tipoNo == No::NUM_NO &&
                    no->direita->tipoNo == No::OP_NO && no->direita->operador == '/' &&
                    no->direita->direita->tipoNo == No::NUM_NO &&
                    abs(no->direita->direita->valor) > 1e-10) {
                    return No::criarOperacao('*',
                        No::criarNumero(no->esquerda->valor / no->direita->direita->valor),
                        no->direita->esquerda);
                }
                // ---- c * (c2 / g)  -->  (c*c2) / g — absorve constante do numerador ----
                if (no->esquerda->tipoNo == No::NUM_NO &&
                    no->direita->tipoNo == No::OP_NO && no->direita->operador == '/' &&
                    no->direita->esquerda->tipoNo == No::NUM_NO &&
                    no->direita->direita->tipoNo != No::NUM_NO) {
                    return No::criarOperacao('/',
                        No::criarNumero(no->esquerda->valor * no->direita->esquerda->valor),
                        copiar(no->direita->direita));
                }
                // ---- f * (c * g)  -->  c * (f * g) — move a constante para a frente ----
                if (no->esquerda->tipoNo != No::NUM_NO &&
                    no->direita->tipoNo == No::OP_NO && no->direita->operador == '*' &&
                    no->direita->esquerda->tipoNo == No::NUM_NO) {
                    return No::criarOperacao('*', no->direita->esquerda,
                        No::criarOperacao('*', no->esquerda, no->direita->direita));
                }
                // ---- (f / g) * g  →  f  — cancela denominador ----
                if (no->esquerda->tipoNo == No::OP_NO && no->esquerda->operador == '/' &&
                    saoIguais(no->esquerda->direita, no->direita)) {
                    return copiar(no->esquerda->esquerda);
                }
                // ---- g * (f / g)  →  f  — idem, comutativo ----
                if (no->direita->tipoNo == No::OP_NO && no->direita->operador == '/' &&
                    saoIguais(no->direita->direita, no->esquerda)) {
                    return copiar(no->direita->esquerda);
                }
                // ---- (f / x^n) * x  →  f / x^(n-1) — cancela um fator de x ----
                if (no->esquerda->tipoNo == No::OP_NO && no->esquerda->operador == '/' &&
                    no->esquerda->direita->tipoNo == No::OP_NO && no->esquerda->direita->operador == '^' &&
                    no->direita->tipoNo == No::VAR_NO) {
                    No* base  = no->esquerda->direita->esquerda;
                    No* expNo = no->esquerda->direita->direita;
                    if (saoIguais(base, no->direita) && expNo->tipoNo == No::NUM_NO && expNo->valor > 1) {
                        double newExp = expNo->valor - 1;
                        No* num = copiar(no->esquerda->esquerda);
                        if (abs(newExp - 1.0) < 1e-10) return No::criarOperacao('/', num, copiar(base));
                        return No::criarOperacao('/', num,
                            No::criarOperacao('^', copiar(base), No::criarNumero(newExp)));
                    }
                }
                // ---- x * (f / x^n)  →  f / x^(n-1) — idem, comutativo ----
                if (no->direita->tipoNo == No::OP_NO && no->direita->operador == '/' &&
                    no->direita->direita->tipoNo == No::OP_NO && no->direita->direita->operador == '^' &&
                    no->esquerda->tipoNo == No::VAR_NO) {
                    No* base  = no->direita->direita->esquerda;
                    No* expNo = no->direita->direita->direita;
                    if (saoIguais(base, no->esquerda) && expNo->tipoNo == No::NUM_NO && expNo->valor > 1) {
                        double newExp = expNo->valor - 1;
                        No* num = copiar(no->direita->esquerda);
                        if (abs(newExp - 1.0) < 1e-10) return No::criarOperacao('/', num, copiar(base));
                        return No::criarOperacao('/', num,
                            No::criarOperacao('^', copiar(base), No::criarNumero(newExp)));
                    }
                }
                break;

            case '/':
                if (saoIguais(no->esquerda, no->direita)) return No::criarNumero(1);
                if (ehZero(no->esquerda)) return No::criarNumero(0); // 0 / g = 0
                if (ehUm(no->direita))  return no->esquerda;        // f / 1 = f
                // f / (-1) = -f
                if (no->direita->tipoNo == No::NUM_NO &&
                    abs(no->direita->valor + 1.0) < 1e-10) {
                    return No::criarOperacao('*', No::criarNumero(-1), no->esquerda);
                }

                // ---- Cortar frações: (c1 * f) / c2  -->  (c1/c2) * f ----
                if (no->esquerda->tipoNo == No::OP_NO && no->esquerda->operador == '*' &&
                    no->esquerda->esquerda->tipoNo == No::NUM_NO &&
                    no->direita->tipoNo == No::NUM_NO) {
                    
                    double c1 = no->esquerda->esquerda->valor; 
                    double c2 = no->direita->valor;            

                    if (abs(c2) > 1e-10) {
                        double divisao = c1 / c2;
                        
                        if (abs(divisao - 1.0) < 1e-10) {
                            return copiar(no->esquerda->direita);
                        } else {
                            return No::criarOperacao('*', No::criarNumero(divisao), copiar(no->esquerda->direita));
                        }
                    }
                }

                break;

            case '^':
                if (ehZero(no->direita)) return No::criarNumero(1); // f ^ 0 = 1
                if (ehUm(no->direita))  return no->esquerda;        // f ^ 1 = f
                break;
        }
    }

    return no;
}

// ---- Conversão para String ----
// Transforma a árvore de expressão em uma string legível para o usuário
// Gerencia parentização baseada na precedência dos operadores
string noParaString(No* no) {
    if (!no) return "0";

    switch (no->tipoNo) {

        // Número: exibe como inteiro se possível, senão com 4 casas decimais
        case No::NUM_NO: {
            // Detecta constantes especiais (π e e) para exibição simbólica
            if (abs(no->valor - PI_VALOR) < 1e-6) return "π";
            if (abs(no->valor - E_VALOR)  < 1e-6) return "e";

            // Se o valor é inteiro, mostra sem casas decimais
            if (no->valor == (int)no->valor && abs(no->valor) < 1e9) {
                return to_string((int)no->valor);
            }
            ostringstream oss;
            oss << fixed << setprecision(4) << no->valor;
            return oss.str();
        }

        // Variável: mostra 'x' ou 'y'
        case No::VAR_NO:
            return string(1, no->variavel);

        // Função: mostra "nome(argumento)"
        case No::FUNC_NO:
            return no->funcao + "(" + noParaString(no->esquerda) + ")";

        // Operação: monta "esquerda op direita" com parênteses quando necessário
        case No::OP_NO: {
            string esq = noParaString(no->esquerda);
            string dir = noParaString(no->direita);

            // Caso especial: (-1) * algo → exibe como "-algo"
            if (no->operador == '*' && no->esquerda->tipoNo == No::NUM_NO &&
                no->esquerda->valor == -1) {
                // Se o filho direito é soma/subtração, coloca parênteses: -(a + b)
                if (no->direita->tipoNo == No::OP_NO &&
                    (no->direita->operador == '+' || no->direita->operador == '-')) {
                    return "-(" + dir + ")";
                }
                return "-" + dir;
            }

            // Parentização do filho esquerdo: se sua precedência é menor que a do pai
            if (no->esquerda->tipoNo == No::OP_NO) {
                int precPai    = precedencia(no->operador);
                int precFilho = precedencia(no->esquerda->operador);
                if (precFilho < precPai) {
                    esq = "(" + esq + ")";
                }
            }

            // Parentização do filho direito: precisa de mais cuidado
            if (no->direita->tipoNo == No::OP_NO) {
                int precPai    = precedencia(no->operador);
                int precFilho = precedencia(no->direita->operador);
                // Parênteses se precedência menor, ou igual mas operação não é comutativa
                if (precFilho < precPai ||
                    (precFilho == precPai &&
                     (no->operador == '-' || no->operador == '/' || no->operador == '^'))) {
                    dir = "(" + dir + ")";
                }
            }

            // Multiplicação implícita: "2x" ao invés de "2*x"
            if (no->operador == '*') {
                bool esqNum = no->esquerda->tipoNo == No::NUM_NO;
                bool dirVar = no->direita->tipoNo == No::VAR_NO;
                bool dirFunc = no->direita->tipoNo == No::FUNC_NO;
                bool dirPot = no->direita->tipoNo == No::OP_NO && no->direita->operador == '^';

                // Número seguido de variável, função ou potência → multiplicação implícita
                if (esqNum && (dirVar || dirFunc || dirPot)) {
                    return esq + dir;       // "2x", "3sin(x)", "2x^2"
                }
                return esq + "*" + dir;     // Caso geral: "sin(x)*cos(x)"
            }

            // Potenciação: usa superscrito se o expoente é um inteiro pequeno
            if (no->operador == '^') {
                if (no->direita->tipoNo == No::NUM_NO) {
                    // Expoente 1/2 → notação de raiz quadrada (ex.: (1-x²)^0.5 → sqrt(1 - x²))
                    if (abs(no->direita->valor - 0.5) < 1e-9) {
                        return "sqrt(" + noParaString(no->esquerda) + ")";
                    }
                    int exp = (int)no->direita->valor;
                    if (no->direita->valor == exp && exp >= 2 && exp <= 9) {
                        return esq + superscrito(exp); // "x²", "cos(x)³"
                    }
                }
                return esq + "^" + dir;     // Expoente genérico: "x^n"
            }

            // Soma com número negativo à direita: "x + -3" → "x - 3"
            if (no->operador == '+' && no->direita->tipoNo == No::NUM_NO &&
                no->direita->valor < 0) {
                No tmp;
                tmp.tipoNo = No::NUM_NO;
                tmp.valor = -no->direita->valor;
                return esq + " - " + noParaString(&tmp);
            }

            // Adição e subtração: espaços ao redor do operador
            return esq + " " + string(1, no->operador) + " " + dir;
        }
    }

    return "?";
}

// ---- Liberação de Memória ----
// Percorre a árvore recursivamente e libera cada nó alocado com new
void liberarArvore(No* no) {
    if (!no) return;
    liberarArvore(no->esquerda); // Libera sub-árvore esquerda primeiro
    liberarArvore(no->direita);  // Libera sub-árvore direita
    delete no;                    // Libera o nó atual
}

// ===================== UTILITÁRIOS ===========================


// ---- Fatorial ----
// Calcula n! usando double para suportar valores grandes sem overflow de int
double fatorial(int n) {
    double resultado = 1.0; // Inicia com 1 (0! = 1! = 1)
    for (int i = 2; i <= n; i++) {
        resultado *= i; // Multiplica acumulativamente: 1 * 2 * 3 * ... * n
    }
    return resultado;
}

// ---- Combinação ----
// Calcula C(n, k) = n! / (k! · (n-k)!) de forma otimizada
double combinacao(int n, int k) {
    if (k > n) return 0;                // C(n, k) = 0 se k > n
    if (k == 0 || k == n) return 1;     // C(n, 0) = C(n, n) = 1
    if (k > n - k) k = n - k;           // Otimização: C(n, k) = C(n, n-k)

    double resultado = 1.0;
    for (int i = 0; i < k; i++) {
        resultado *= (n - i);            // Numerador: n · (n-1) · ... · (n-k+1)
        resultado /= (i + 1);           // Denominador: k!
    }
    return resultado;
}

// ---- Superscrito Unicode ----
// Converte um número inteiro em seus dígitos superscrito Unicode
// Exemplo: 2 → "²", 10 → "¹⁰", 23 → "²³"
string superscrito(int num) {
    // Array com os caracteres Unicode de superscrito para cada dígito
    const string digitos[] = {
        "\u2070", "\u00B9", "\u00B2", "\u00B3", "\u2074",
        "\u2075", "\u2076", "\u2077", "\u2078", "\u2079"
    };

    if (num < 0) return "\u207B" + superscrito(-num); // Superscrito negativo (⁻)

    // Converte o número para string e mapeia cada dígito para superscrito
    string numStr = to_string(num);
    string resultado = "";
    for (int i = 0; i < (int)numStr.size(); i++) {
        int d = numStr[i] - '0';   // Converte char para índice numérico
        resultado += digitos[d];    // Concatena o caractere superscrito correspondente
    }
    return resultado;
}
