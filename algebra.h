// ============================================================
//                        ALGEBRA.H
// Biblioteca de ferramentas matemáticas para Cálculo 1 e 2
//
// Contém:
//   - Parser de expressões matemáticas (tokenização + pós-fixo)
//   - Métodos numéricos para Cálculo 1 (limites, derivadas, integrais)
//   - Métodos numéricos para Cálculo 2 (Taylor, Maclaurin, EDO)
//   - Funções utilitárias (fatorial, combinação, superscrito)
// ============================================================

#ifndef ALGEBRA_H  // Guarda de inclusão — evita inclusão duplicada
#define ALGEBRA_H

#include <iostream>    // Entrada e saída padrão (cin para ler, cout para escrever no terminal)
#include <string>      // Classe string — texto dinâmico (a expressão digitada, nomes de funções)
#include <vector>      // Classe vector — vetor/lista dinâmica (tokens, coeficientes de Taylor)
#include <cmath>       // Funções matemáticas: sin, cos, tan, pow, exp, log, sqrt, fabs, isnan...
#include <stack>       // Classe stack — pilha (LIFO), usada na avaliação da notação pós-fixa
#include <sstream>     // ostringstream/istringstream — converter número ↔ string sem exceções
#include <iomanip>     // Manipuladores de formatação de saída: setprecision, setw, fixed
#include <cctype>      // Classificação de caracteres: isdigit, isalpha, isspace, tolower
#include <algorithm>   // Funções utilitárias de algoritmos: max e min (comparações numéricas)
#include <fstream>     // Arquivos: ofstream/ifstream para texto, CSV e binário (tópico IX)

using namespace std;

// ======================== CONSTANTES ========================
const double PI_VALOR = 3.14159265358979323846;  // Valor de π com 20 dígitos
const double E_VALOR  = 2.71828182845904523536;  // Valor do número de Euler
const double EPSILON  = 1e-9;                     // Tolerância para comparações numéricas
const char OP_NEGACAO = '~';  // Menos unário (ex.: -x), um operador diferente da subtração

// ====================== ESTRUTURA TOKEN ======================
// Representa uma unidade léxica da expressão matemática
struct Token {
    // Enumeração dos tipos possíveis de token
    // INVALIDO guarda um trecho não reconhecido, para validarSintaxe poder recusá-lo
    enum Tipo { NUMERO, OPERADOR, FUNCAO, VARIAVEL, ABRE_PAREN, FECHA_PAREN, INVALIDO };

    Tipo tipo;         // Tipo do token (NUMERO, OPERADOR, etc.)
    double valor;      // Armazena o valor numérico (usado quando tipo == NUMERO)
    char operador;     // Armazena o caractere do operador (usado quando tipo == OPERADOR)
    string funcao;     // Nome da função (FUNCAO) ou o trecho recusado (INVALIDO)
    char variavel;     // Armazena 'x' ou 'y' (usado quando tipo == VARIAVEL)
    bool nomeada;      // True se o NUMERO veio de uma constante nomeada (pi, e)

    // Construtor padrão — inicializa todos os campos com valores neutros
    Token() : tipo(NUMERO), valor(0), operador(0), variavel(0), nomeada(false) {}

    // Métodos estáticos de fábrica — criam tokens de cada tipo
    static Token numero(double v);
    static Token constante(double v);
    static Token invalido(const string& trecho);
    static Token op(char c);
    static Token func(const string& nome);
    static Token var(char c);
    static Token abreParen();
    static Token fechaParen();
};

// ========================= PARSER ============================
// Converte a string da expressão em tokens, resolve precedência e avalia

vector<Token> tokenizar(const string& expressao);              // Quebra a expressão em tokens
bool validarSintaxe(const vector<Token>& tokens, string& erro); // Valida sintaxe; false + msg de erro se inválida
bool contemVariavel(const vector<Token>& tokens, char var);     // True se os tokens contêm a variável 'var'
vector<Token> inserirMultImplicita(const vector<Token>& tokens); // Insere '*' implícito (ex: 3x → 3*x)
vector<Token> paraPosfixo(const vector<Token>& tokens);        // Converte infixa → pós-fixa (Shunting-yard)
int precedencia(char op);                                       // Retorna precedência do operador
bool ehAssocEsquerda(char op);                                  // Verifica se é associativo à esquerda
double avaliar(const vector<Token>& posfixo, double x, double y = 0.0); // Avalia a expressão pós-fixa
double avaliarFuncao(const string& nome, double argumento);     // Avalia uma função matemática (sin, cos, etc.)

// ====================== CÁLCULO 1 ===========================
// Métodos numéricos para Limites, Continuidade, Derivadas e Integrais

double limiteEsquerda(const vector<Token>& f, double a);       // Limite lateral pela esquerda
double limiteDireita(const vector<Token>& f, double a);        // Limite lateral pela direita
double limite(const vector<Token>& f, double a);               // Limite bilateral
bool verificarContinuidade(const vector<Token>& f, double a);  // Verifica continuidade no ponto
double derivada(const vector<Token>& f, double x);             // Primeira derivada (diferença central)
double derivadaSegunda(const vector<Token>& f, double x);      // Segunda derivada
double derivadaImplicita(const vector<Token>& F, double x0, double y0);    // dy/dx para F(x,y)=0
double integralDefinida(const vector<Token>& f, double a, double b, int n = 100000); // Regra de Simpson

// Situação de uma integral numérica
enum StatusIntegral {
    INTEGRAL_OK,         // valor confiável
    INTEGRAL_IMPRECISA,  // há valor, mas com precisão limitada
    INTEGRAL_DIVERGE,    // integral imprópria divergente
    INTEGRAL_INCERTA,    // singularidade forte demais para decidir
    INTEGRAL_INDEFINIDA  // f não existe (ou estoura) em parte do intervalo
};
struct ResultadoIntegral {
    StatusIntegral status;
    double valor;  // válido para INTEGRAL_OK e INTEGRAL_IMPRECISA
    double ponto;  // onde está o problema, quando houver
};
ResultadoIntegral integrarNumericamente(const vector<Token>& f, double a, double b,
                                        int n = 100000); // Simpson + tratamento de singularidades
void mostrarIntegralIndefinida(const vector<Token>& f, double xMin, double xMax, int pontos = 20,
                               const string& arquivoCSV = ""); // Tabela de F(x); exporta CSV se nome != ""
double newtonRaphson(const vector<Token>& f, double x0, int& iteracoes);   // Raiz de f(x)=0 (Newton-Raphson)
void acharPontosCriticos(const vector<Token>& f, double a, double b);      // Máx/mín/inflexão em [a,b]

// ====================== CÁLCULO 2 ===========================
// Métodos numéricos para Séries de Taylor/Maclaurin e EDOs

vector<double> calcularCoefsTaylor(const vector<Token>& f, double a, int numTermos); // Coeficientes aₙ
double avaliarSerieTaylor(const vector<double>& coefs, double a, double x);          // P(x) numérico
string formatarSerieTaylor(const vector<double>& coefs, double a);                   // P(x) string

// Integral definida calculada integrando a série de Taylor termo a termo
struct IntegralTaylor {
    bool convergiu;
    double valor;
    int pedacos;           // em quantas partes [a, b] precisou ser dividido
    int grau;              // maior grau de termo que ainda influenciou o resultado
    double centro;         // centro da série (quando pedacos == 1)
    vector<double> coefs;  // coeficientes de f em torno do centro (quando pedacos == 1)
};
IntegralTaylor integrarPorTaylor(const vector<Token>& f, double a, double b);
string formatarPrimitivaSerie(const vector<double>& coefs, double centro, int maxTermos); // F(x) ≈ ...
void resolverEDOEuler(const vector<Token>& f, double x0, double y0, double xf, int numPassos,
                      const string& arquivoCSV = "");                                // Euler; exporta CSV se nome != ""

// ---- Derivadas parciais e séries (Cálculo 2) ----
double derivada(const vector<Token>& f, double x, double y, char var); // SOBRECARGA: ∂f/∂var em (x,y)
enum Veredito { SERIE_CONVERGE, SERIE_DIVERGE, SERIE_INCONCLUSIVA };
bool termoNaoTendeAZero(const vector<Token>& termo);           // Teste do termo geral
double testeRazaoSerie(const vector<Token>& termo);            // L = lim |a(n+1)/a(n)| (NAN se não se aplica)
Veredito analisarDecaimento(const vector<Token>& termo, string& motivo); // Quando a razão não decide
double somaParcialSerie(const vector<Token>& termo, int n);    // S(n) = a(1)+...+a(n)

// ================ MANIPULAÇÃO DE ARQUIVOS ====================
// Salva/lê os coeficientes de Taylor em arquivo BINÁRIO (tópico IX da ementa)
bool salvarCoeficientesBin(const string& nomeArquivo, const vector<double>& coefs, double centro);
bool carregarCoeficientesBin(const string& nomeArquivo, vector<double>& coefs, double& centro);

// ================ DIFERENCIAÇÃO SIMBÓLICA ====================
// Representa a expressão como árvore binária para derivação algébrica

struct No {
    // Enumeração dos tipos de nó da árvore
    enum TipoNo { NUM_NO, VAR_NO, OP_NO, FUNC_NO };

    TipoNo tipoNo;        // Tipo do nó (número, variável, operador ou função)
    double valor;          // Valor numérico (quando tipoNo == NUM_NO)
    char operador;         // Caractere do operador (quando tipoNo == OP_NO)
    char variavel;         // 'x' ou 'y' (quando tipoNo == VAR_NO)
    string funcao;         // Nome da função (quando tipoNo == FUNC_NO)
    No* esquerda;          // Ponteiro para o filho esquerdo (primeiro operando / argumento)
    No* direita;           // Ponteiro para o filho direito (segundo operando, nullptr para funções)

    // Construtor padrão — inicializa ponteiros como nullptr
    No() : tipoNo(NUM_NO), valor(0), operador(0), variavel(0),
           esquerda(nullptr), direita(nullptr) {}

    // Métodos estáticos de fábrica para criar cada tipo de nó
    static No* criarNumero(double v);
    static No* criarVariavel(char c);
    static No* criarOperacao(char op, No* esq, No* dir);
    static No* criarFuncao(const string& nome, No* arg);
};

No* construirArvore(const vector<Token>& posfixo);   // Monta a árvore a partir dos tokens pós-fixos
No* copiar(No* no);                                   // Cópia profunda de uma sub-árvore
bool saoIguais(No* a, No* b);                         // Verifica equivalência estrutural
No* diferenciar(No* no, char var = 'x');              // Calcula f'(x) simbolicamente
No* integrar(No* no, char var = 'x');                 // Calcula ∫f(x)dx simbolicamente (nullptr se não conseguir)
No* simplificar(No* no);                              // Simplifica a expressão (x+0=x, 1*x=x, etc.)
string noParaString(No* no);                           // Converte a árvore para string legível
void liberarArvore(No* no);                            // Libera memória da árvore

// ===================== UTILITÁRIOS ===========================
double fatorial(int n);          // Calcula n! usando double para evitar overflow
double combinacao(int n, int k); // Calcula C(n,k) = n! / (k! * (n-k)!)
string superscrito(int num);     // Converte dígito em superscrito Unicode (ex: 2 → ²)

#endif // ALGEBRA_H
