// ============================================================
// Programa principal — Calculadora interativa de Cálculo 1 e 2
//
// Utiliza a biblioteca algebra.h para parsing de expressões e
// métodos numéricos. O usuário digita uma função f(x) e depois
// seleciona operações de Cálculo 1 ou Cálculo 2 via menu.
// ============================================================

#include <iostream>    // Entrada e saída padrão (cin/cout) para o menu interativo
#include <string>      // Classe string — guarda a expressão digitada pelo usuário
#include <vector>      // Classe vector — vetor de tokens da expressão (pós-fixa)
#include <iomanip>     // Formatação da saída numérica (setprecision, fixed, setw)
#include <limits>      // numeric_limits — usado em cin.ignore() e na leitura robusta de inteiros
#include <windows.h>   // API do Windows: SetConsoleOutputCP(CP_UTF8) para exibir acentos/símbolos
#include "algebra.h"   // Biblioteca própria do projeto (parser, métodos numéricos, AST)

using namespace std;

// Processa uma expressão: tokeniza, insere mult. implícita, valida e converte para
// pós-fixa. Retorna true e preenche 'posfixo'; em caso de erro, retorna false e
// descreve o problema em 'erro' (passagem por referência). Não usa exceções.
// 'varsObrigatorias' lista as variáveis que a expressão DEVE conter (ex: "x");
// se vazia, aceita expressões constantes (usado por lerValor para pi/2, sqrt(2)...).
static bool processarExpressao(const string& expr, vector<Token>& posfixo, string& erro,
                               const string& varsObrigatorias = "") {
    vector<Token> tks = inserirMultImplicita(tokenizar(expr));
    if (!validarSintaxe(tks, erro)) return false;

    // Garante que a expressão envolve a(s) variável(is) adequada(s)
    for (size_t i = 0; i < varsObrigatorias.size(); i++) {
        char v = varsObrigatorias[i];
        if (!contemVariavel(tks, v)) {
            erro = string("A expressão precisa ser uma função de '") + v +
                   "' (deve conter a variável '" + v + "').";
            return false;
        }
    }

    posfixo = paraPosfixo(tks);
    return true;
}

// Lê um valor numérico aceitando expressões constantes como pi/2, sqrt(2), 3*pi.
// Lê a linha inteira e pede de novo enquanto a entrada for inválida.
// Retorna NAN apenas se a entrada acabar.
static double lerValor() {
    string s;
    while (getline(cin >> ws, s)) {
        // Número simples, inclusive notação científica (1e-3): precisa consumir tudo
        double v = 0.0;
        istringstream iss(s);
        if (iss >> v && (iss >> ws).eof()) return v;

        vector<Token> pos;
        string erro;
        if (processarExpressao(s, pos, erro)) {
            // processarExpressao não exige variáveis; aqui elas são proibidas
            vector<Token> tks = tokenizar(s);
            if (contemVariavel(tks, 'x') || contemVariavel(tks, 'y')) {
                erro = "O valor não pode conter variáveis.";
            } else {
                v = avaliar(pos, 0.0);
                if (!isnan(v) && !isinf(v)) return v;
                erro = "A expressão não resulta em um número finito.";
            }
        }
        cout << "  " << erro << "\n  Digite o valor novamente: ";
    }
    return NAN;
}

// Lê um número inteiro de forma robusta: se o usuário digitar letras/símbolos,
// limpa o estado de erro do cin e pede novamente (em vez de encerrar sozinho).
static int lerInteiro() {
    int valor;
    while (!(cin >> valor)) {
        if (cin.eof()) return 0;   // sem mais entrada: trata como "Sair"
        cin.clear();               // limpa o estado de falha do fluxo
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // descarta a linha inválida
        cout << "  Entrada inválida! Digite um número inteiro: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // descarta o resto da linha
    return valor;
}

// Formata uma soma parcial para exibição, com guarda contra overflow (números enormes)
static string fmtSoma(double v) {
    if (isnan(v)) return "indefinido";
    if (isinf(v) || fabs(v) > 1e15) return "muito grande (termos crescem)";
    ostringstream oss;
    oss << fixed << setprecision(6) << v;
    return oss.str();
}

// Pergunta de sim/não; retorna true para 's' ou 'S'
static bool perguntarSim(const string& pergunta) {
    char resp;
    cout << pergunta;
    cin >> resp;
    return (resp == 's' || resp == 'S');
}

// Monta a linha "∫[a, b] f(x) dx = valor", ou o motivo de não haver valor
static string descreverIntegral(double a, double b, const ResultadoIntegral& r) {
    ostringstream oss;
    oss << "∫[" << a << ", " << b << "] f(x) dx";
    switch (r.status) {
        case INTEGRAL_OK:
            oss << " = " << fixed << setprecision(6) << r.valor;
            break;
        case INTEGRAL_IMPRECISA:
            oss << " ≈ " << fixed << setprecision(6) << r.valor << " (precisão limitada)";
            break;
        case INTEGRAL_DIVERGE:
            oss << " DIVERGE (singularidade em x ≈ " << r.ponto << ")";
            break;
        case INTEGRAL_INCERTA:
            oss << ": não foi possível decidir. Há uma singularidade forte em x ≈ "
                << r.ponto << " e a integral pode divergir.";
            break;
        case INTEGRAL_INDEFINIDA:
            oss << ": f não está definida (ou é grande demais) em parte do intervalo";
            if (!isnan(r.ponto)) oss << ", perto de x = " << r.ponto;
            oss << ".";
            break;
    }
    return oss.str();
}

// Para integrais sem primitiva elementar conhecida: pede os limites, integra a
// série de Taylor de f termo a termo e confere com o método numérico.
// Retorna o texto do resultado, para o relatório.
static string integrarComTaylor(const vector<Token>& f) {
    cout << "  Limite inferior (a): ";
    double a = lerValor();
    cout << "  Limite superior (b): ";
    double b = lerValor();

    ResultadoIntegral numerico = integrarNumericamente(f, a, b);
    if (numerico.status != INTEGRAL_OK && numerico.status != INTEGRAL_IMPRECISA) {
        // Sem integral não há o que aproximar
        string linha = descreverIntegral(a, b, numerico);
        cout << "\n  " << linha << endl;
        return linha;
    }

    IntegralTaylor serie = integrarPorTaylor(f, a, b);
    if (!serie.convergiu) {
        cout << "\n  A série de Taylor não serve para todo o intervalo: há uma singularidade,\n"
             << "  f não é suave (ex.: abs) ou o intervalo é grande demais.\n"
             << "  Pelo método numérico:" << endl;
        string linha = descreverIntegral(a, b, numerico);
        cout << "  " << linha << endl;
        return linha;
    }

    ostringstream oss;
    if (serie.pedacos == 1) {
        oss << "Série de Taylor em torno de x = " << serie.centro << ", integrada termo a termo:\n  "
            << formatarPrimitivaSerie(serie.coefs, serie.centro, 5);
    } else {
        oss << "O intervalo foi dividido em " << serie.pedacos
            << " partes, cada uma com sua série de Taylor.";
    }
    oss << "\n  ∫[" << a << ", " << b << "] f(x) dx ≈ " << fixed << setprecision(8) << serie.valor
        << "  (termos até o grau " << serie.grau << ")";
    oss << "\n  Conferência pela Regra de Simpson: " << numerico.valor;
    if (fabs(serie.valor - numerico.valor) > 1e-6 * max(1.0, fabs(numerico.valor))) {
        oss << "\n  Atenção: os dois métodos discordam; confira o resultado com cuidado.";
    }
    cout << "\n  " << oss.str() << endl;
    return oss.str();
}

// Acrescenta (append) um bloco de resultado ao arquivo de texto "resultados.txt"
// (manipulação de arquivo de texto — tópico IX da ementa).
static void salvarRelatorio(const string& titulo, const string& conteudo) {
    ofstream arq("resultados.txt", ios::app); // ios::app = acrescenta ao final
    if (!arq) {
        cout << "  Erro: não foi possível abrir o arquivo resultados.txt\n";
        return;
    }
    arq << "===== " << titulo << " =====\n" << conteudo << "\n";
    arq.close();
    cout << "  Resultado salvo em resultados.txt\n";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    // ==================== BANNER INICIAL ====================
    cout << "\n";
    cout << "  ══════════════════════════════════════════════════════" << endl;
    cout << "       CALCULADORA DE CÁLCULO 1 E CÁLCULO 2            " << endl;
    cout << "  ══════════════════════════════════════════════════════" << endl;
    cout << "\n  Funções suportadas: sin, cos, tan, ln, log, exp,";
    cout << "\n  sqrt, abs, sec, csc, cot, sinh, cosh, tanh";
    cout << "\n  Operadores: +  -  *  /  ^";
    cout << "\n  Constantes: pi, e";
    cout << "\n  Variáveis: x (e y para derivada implícita / EDO)\n";
    cout << "\n  Exemplos: 3*x^2 + sin(x)   |   e^(-x)   |   ln(x)/x\n" << endl;

    // ==================== ENTRADA DA FUNÇÃO ====================
    string expressao;        // String que armazena a expressão digitada pelo usuário
    vector<Token> posfixo;
    
    while (true) {
        cout << "  Digite a função f(x): ";
        getline(cin, expressao);

        // Processa a expressão; precisa ser uma função de x. Se inválida, repete
        string erro;
        if (processarExpressao(expressao, posfixo, erro, "x")) break;
        cout << "  " << erro << "\n\n";
    }

    // ==================== LOOP PRINCIPAL ====================
    bool rodando = true;    // Flag que mantém o programa ativo (tipo bool)
    int opcao;              // Variável para a opção do menu principal (tipo int)

    while (rodando) {
        // Exibe o menu principal com a função atual
        cout << "\n  ══════════════════════════════════════════════════════" << endl;
        cout << "  Função atual: f(x) = " << expressao << endl;
        cout << "  ══════════════════════════════════════════════════════" << endl;
        cout << "  [1] Cálculo 1 (Limites, Continuidade, Derivadas, Integrais)" << endl;
        cout << "  [2] Cálculo 2 (Séries de Taylor/Maclaurin, EDO)" << endl;
        cout << "  [3] Alterar função" << endl;
        cout << "  [0] Sair" << endl;
        cout << "  ══════════════════════════════════════════════════════" << endl;

        // Leitura com do-while para validação de entrada (garante execução mínima de 1 vez)
        bool valido;  // Flag booleana para validação
        do {
            cout << "  Opção: ";
            opcao = lerInteiro();
            // Usa operadores relacionais (>= e <=) e lógico AND (&&) para validar
            valido = (opcao >= 0 && opcao <= 3);
            // Usa operador lógico NOT (!) para verificar se a opção é inválida
            if (!valido) {
                cout << "  Opção inválida! Escolha entre 0 e 3." << endl;
            }
        } while (!valido); // do-while garante que o bloco roda pelo menos uma vez

        // ==================== SWITCH DO MENU PRINCIPAL ====================
        switch (opcao) {

            // ============================================================
            //                       CÁLCULO 1
            // ============================================================
            case 1: {
                cout << "\n  ────────────────────────────────────────" << endl;
                cout << "              MENU CÁLCULO 1              " << endl;
                cout << "  ────────────────────────────────────────" << endl;
                cout << "  [1] Limite" << endl;
                cout << "  [2] Continuidade" << endl;
                cout << "  [3] Derivada" << endl;
                cout << "  [4] Derivada Implícita" << endl;
                cout << "  [5] Integral Indefinida" << endl;
                cout << "  [6] Integral Definida" << endl;
                cout << "  [7] Raízes (Newton-Raphson)" << endl;
                cout << "  [8] Pontos Críticos (máx/mín)" << endl;
                cout << "  [0] Voltar" << endl;
                cout << "  ────────────────────────────────────────" << endl;

                int op_c1; // Opção do submenu de Cálculo 1
                bool valido_c1;
                do {
                    cout << "  Opção: ";
                    op_c1 = lerInteiro();
                    valido_c1 = (op_c1 >= 0 && op_c1 <= 8);
                    if (!valido_c1) {
                        cout << "  Opção inválida! Escolha entre 0 e 8." << endl;
                    }
                } while (!valido_c1);

                switch (op_c1) {

                    // ──────── LIMITE (SIMBÓLICO) ────────
                    case 1: {
                        // Exibe a função e sua derivada simbolicamente
                        No* arvore = construirArvore(posfixo);
                        No* d1Arvore = diferenciar(arvore);
                        for (int s = 0; s < 5; s++) d1Arvore = simplificar(d1Arvore);
                        string d1Str = noParaString(d1Arvore);

                        cout << "\n  ── Resultado Simbólico ──" << endl;
                        cout << "  f(x)  = " << expressao << endl;
                        cout << "  f'(x) = " << d1Str << "  (para análise de pontos críticos)" << endl;

                        // Pergunta se deseja calcular o limite numérico
                        char substituirLim; // Variável char para a resposta
                        cout << "\n  Deseja substituir um valor para x e calcular o limite? (s/n): ";
                        cin >> substituirLim;

                        if (substituirLim == 's' || substituirLim == 'S') {
                            double a; // Ponto para o qual x tende
                            cout << "  Calcular lim f(x) quando x tende a: ";
                            a = lerValor();

                            double limEsq = limiteEsquerda(posfixo, a); // Limite pela esquerda
                            double limDir = limiteDireita(posfixo, a);  // Limite pela direita
                            double lim = limite(posfixo, a);             // Limite bilateral

                            cout << "\n  ── Resultados Numéricos em x → " << a << " ──" << endl;

                            // Exibe limite pela esquerda
                            cout << "  Limite pela esquerda (x → " << a << "⁻): ";
                            if (isinf(limEsq)) cout << (limEsq > 0 ? "+∞" : "-∞") << endl;
                            else if (isnan(limEsq)) cout << "indeterminado" << endl;
                            else cout << fixed << setprecision(6) << limEsq << endl;

                            // Exibe limite pela direita
                            cout << "  Limite pela direita  (x → " << a << "⁺): ";
                            if (isinf(limDir)) cout << (limDir > 0 ? "+∞" : "-∞") << endl;
                            else if (isnan(limDir)) cout << "indeterminado" << endl;
                            else cout << fixed << setprecision(6) << limDir << endl;

                            // Exibe limite bilateral
                            cout << "\n  Limite bilateral     (x → " << a << "): ";
                            if (isinf(lim)) cout << (lim > 0 ? "+∞" : "-∞") << endl;
                            else if (isnan(lim)) cout << "NÃO EXISTE (limites laterais divergem)" << endl;
                            else cout << fixed << setprecision(6) << lim << endl;
                        }

                        // Oferece salvar f(x) e f'(x) em arquivo de texto
                        if (perguntarSim("\n  Salvar resultado em arquivo? (s/n): ")) {
                            salvarRelatorio("Limite / Derivada simbólica",
                                            "f(x)  = " + expressao + "\nf'(x) = " + d1Str);
                        }

                        liberarArvore(arvore);    // Libera a memória das árvores criadas
                        liberarArvore(d1Arvore);
                        break;
                    }

                    // ──────── CONTINUIDADE ────────
                    case 2: {
                        double a; // Ponto onde verificar continuidade
                        cout << "\n  Verificar continuidade no ponto x = ";
                        a = lerValor();
                        verificarContinuidade(posfixo, a); // Exibe diagnóstico completo
                        break;
                    }

                    // ──────── DERIVADA (SIMBÓLICA) ────────
                    case 3: {
                        // Constrói a árvore de expressão a partir dos tokens pós-fixos
                        No* arvore = construirArvore(posfixo);

                        // Calcula a primeira derivada simbolicamente
                        No* d1Arvore = diferenciar(arvore);
                        // Aplica simplificação múltiplas vezes para resultado mais limpo
                        for (int s = 0; s < 5; s++) d1Arvore = simplificar(d1Arvore);
                        string d1Str = noParaString(d1Arvore); // Converte para string legível

                        // Calcula a segunda derivada simbolicamente (derivada da derivada)
                        No* d2Arvore = diferenciar(d1Arvore);
                        for (int s = 0; s < 5; s++) d2Arvore = simplificar(d2Arvore);
                        string d2Str = noParaString(d2Arvore);

                        // Exibe os resultados simbólicos
                        cout << "\n  ── Resultado Simbólico ──" << endl;
                        cout << "  f(x)   = " << expressao << endl;
                        cout << "  f'(x)  = " << d1Str << endl;
                        cout << "  f''(x) = " << d2Str << endl;

                        // Pergunta se deseja substituir x por um valor numérico
                        char substituirDeriv; // Variável char para a resposta
                        cout << "\n  Deseja substituir um valor para x? (s/n): ";
                        cin >> substituirDeriv;

                        // Usa operador lógico OR (||) para aceitar 's' ou 'S'
                        if (substituirDeriv == 's' || substituirDeriv == 'S') {
                            double x0; // Valor de x para substituição
                            cout << "  Valor de x = ";
                            x0 = lerValor();

                            double fx = avaliar(posfixo, x0);          // f(x₀)
                            double d1 = derivada(posfixo, x0);         // f'(x₀) numérico
                            double d2 = derivadaSegunda(posfixo, x0);  // f''(x₀) numérico

                            cout << "\n  ── Resultados Numéricos em x = " << x0 << " ──" << endl;
                            cout << "  f(" << x0 << ")   = " << fixed << setprecision(6) << fx << endl;
                            cout << "  f'(" << x0 << ")  = " << d1 << endl;
                            cout << "  f''(" << x0 << ") = " << d2 << endl;

                            // Interpretação geométrica da segunda derivada
                            if (d2 > EPSILON) {
                                cout << "\n  Concavidade para CIMA no ponto (f'' > 0)" << endl;
                            } else if (d2 < -EPSILON) {
                                cout << "\n  Concavidade para BAIXO no ponto (f'' < 0)" << endl;
                            } else {
                                cout << "\n  Possível ponto de inflexão (f'' ≈ 0)" << endl;
                            }
                        }

                        // Oferece salvar o resultado em arquivo de texto
                        if (perguntarSim("\n  Salvar resultado em arquivo? (s/n): ")) {
                            string conteudo = "f(x)   = " + expressao + "\n" +
                                              "f'(x)  = " + d1Str + "\n" +
                                              "f''(x) = " + d2Str;
                            salvarRelatorio("Derivada", conteudo);
                        }

                        liberarArvore(arvore);    // Libera as três árvores (f, f', f'')
                        liberarArvore(d1Arvore);
                        liberarArvore(d2Arvore);
                        break;
                    }

                    // ──────── DERIVADA IMPLÍCITA ────────
                    case 4: {
                        cout << "\n  ── Derivada Implícita ──" << endl;
                        cout << "  Para F(x, y) = 0, calcula dy/dx = -(∂F/∂x)/(∂F/∂y)\n" << endl;

                        string exprImplicita;
                        vector<Token> posfixoImpl;

                        while (true) {
                            cout << "  Digite a expressão F(x, y) (ex: x^2 + y^2 - 25): ";
                            getline(cin, exprImplicita); // Lê a expressão com duas variáveis

                            // Processa F(x, y); precisa conter y (∂F/∂y vai ao denominador)
                            string erro;
                            if (processarExpressao(exprImplicita, posfixoImpl, erro, "y")) break;
                            cout << "  " << erro << "\n\n";
                        }

                        double x0, y0; // Ponto (x₀, y₀) onde calcular dy/dx
                        cout << "  Valor de x₀: ";
                        x0 = lerValor();
                        cout << "  Valor de y₀: ";
                        y0 = lerValor();

                        // Verifica se o ponto satisfaz F(x₀, y₀) ≈ 0
                        double Fxy = avaliar(posfixoImpl, x0, y0);
                        cout << "\n  F(" << x0 << ", " << y0 << ") = " << fixed << setprecision(6) << Fxy;
                        if (abs(Fxy) > 0.01) {
                            cout << "  (o ponto não está exatamente sobre a curva)" << endl;
                        } else {
                            cout << "  ≈ 0 (ponto sobre a curva)" << endl;
                        }

                        double dydx = derivadaImplicita(posfixoImpl, x0, y0); // Calcula dy/dx
                        cout << "  dy/dx em (" << x0 << ", " << y0 << ") = " << dydx << endl;
                        break;
                    }

                    // ──────── INTEGRAL INDEFINIDA (SIMBÓLICA) ────────
                    case 5: {
                        // Tenta calcular a antiderivada simbolicamente
                        No* arvore = construirArvore(posfixo);
                        No* intArvore = integrar(arvore);

                        cout << "\n  ── Resultado Simbólico ──" << endl;
                        cout << "  f(x)    = " << expressao << endl;

                        string resultadoInt; // guarda o texto do resultado para salvar
                        if (intArvore) {
                            for (int s = 0; s < 5; s++) intArvore = simplificar(intArvore);
                            resultadoInt = noParaString(intArvore) + " + C";
                            cout << "  ∫f(x)dx = " << resultadoInt << endl;
                        } else {
                            resultadoInt = "(primitiva em funções elementares não encontrada)";
                            cout << "  ∫f(x)dx = " << resultadoInt << endl;
                            if (perguntarSim("\n  Deseja calcular valores numéricos usando série de Taylor? (s/n): "))
                                resultadoInt += "\n  " + integrarComTaylor(posfixo);
                        }

                        // Pergunta se deseja substituir x para ver a tabela numérica
                        if (perguntarSim("\n  Deseja substituir valores para x e gerar a tabela numérica? (s/n): ")) {
                            double xMin, xMax; // Intervalo para exibir a tabela
                            cout << "  x mínimo: ";
                            xMin = lerValor();
                            cout << "  x máximo: ";
                            xMax = lerValor();
                            // Pergunta se quer exportar a tabela para CSV
                            string csv = "";
                            if (perguntarSim("  Exportar a tabela para CSV (tabela_integral.csv)? (s/n): "))
                                csv = "tabela_integral.csv";
                            mostrarIntegralIndefinida(posfixo, xMin, xMax, 20, csv);
                        }

                        // Oferece salvar o resultado simbólico em arquivo de texto
                        if (perguntarSim("\n  Salvar resultado em arquivo? (s/n): ")) {
                            salvarRelatorio("Integral Indefinida",
                                            "f(x)    = " + expressao + "\n∫f(x)dx = " + resultadoInt);
                        }

                        liberarArvore(arvore);     // Libera a árvore de f e a da antiderivada
                        liberarArvore(intArvore);  // (liberarArvore(nullptr) é seguro)
                        break;
                    }

                    // ──────── INTEGRAL DEFINIDA (SIMBÓLICA) ────────
                    case 6: {
                        // Tenta calcular a antiderivada simbolicamente
                        No* arvore = construirArvore(posfixo);
                        No* intArvore = integrar(arvore);

                        cout << "\n  ── Resultado Simbólico ──" << endl;
                        cout << "  f(x)    = " << expressao << endl;

                        string resultadoDef; // texto do resultado simbólico para salvar
                        if (intArvore) {
                            for (int s = 0; s < 5; s++) intArvore = simplificar(intArvore);
                            resultadoDef = noParaString(intArvore) + " + C";
                            cout << "  ∫f(x)dx = " << resultadoDef << endl;
                            cout << "  (Pela regra de Barrow: ∫[a,b] f(x)dx = F(b) - F(a))" << endl;
                        } else {
                            resultadoDef = "(primitiva em funções elementares não encontrada)";
                            cout << "  ∫f(x)dx = " << resultadoDef << endl;
                        }

                        string conteudoDef = "f(x)    = " + expressao + "\n∫f(x)dx = " + resultadoDef;

                        // Sem primitiva, oferece primeiro o cálculo por série de Taylor
                        if (!intArvore &&
                            perguntarSim("\n  Deseja calcular valores numéricos usando série de Taylor? (s/n): ")) {
                            conteudoDef += "\n" + integrarComTaylor(posfixo);
                        }
                        // Pergunta se deseja calcular com limites específicos
                        else if (perguntarSim("\n  Deseja substituir os limites de integração? (s/n): ")) {
                            double a, b; // Limites de integração
                            cout << "  Limite inferior (a): ";
                            a = lerValor();
                            cout << "  Limite superior (b): ";
                            b = lerValor();

                            // ∫[a,b] f(x)dx, com verificação de divergência
                            string linha = descreverIntegral(a, b, integrarNumericamente(posfixo, a, b));
                            cout << "\n  " << linha << endl;

                            // Acrescenta o resultado ao conteúdo a ser salvo
                            conteudoDef += "\n" + linha;
                        }

                        // Oferece salvar o resultado em arquivo de texto
                        if (perguntarSim("\n  Salvar resultado em arquivo? (s/n): ")) {
                            salvarRelatorio("Integral Definida", conteudoDef);
                        }

                        liberarArvore(arvore);     // Libera a árvore de f e a da antiderivada
                        liberarArvore(intArvore);
                        break;
                    }

                    // ──────── RAÍZES (NEWTON-RAPHSON) ────────
                    case 7: {
                        cout << "\n  ── Raízes: Método de Newton-Raphson ──" << endl;
                        cout << "  Resolve f(x) = 0 a partir de um chute inicial, usando f'(x).\n" << endl;
                        cout << "  Chute inicial x₀ = ";
                        double x0 = lerValor();

                        int iters = 0;
                        double raiz = newtonRaphson(posfixo, x0, iters);

                        if (isnan(raiz)) {
                            cout << "\n  Não convergiu a partir de x₀ = " << x0
                                 << " (tente outro chute inicial)." << endl;
                        } else {
                            cout << "\n  Raiz encontrada: x ≈ " << fixed << setprecision(6) << raiz
                                 << "  (em " << iters << " iterações)" << endl;
                            cout << "  Verificação: f(" << raiz << ") = " << avaliar(posfixo, raiz) << endl;

                            if (perguntarSim("\n  Salvar resultado em arquivo? (s/n): ")) {
                                ostringstream oss;
                                oss << "f(x) = " << expressao << "\nRaiz: x = "
                                    << fixed << setprecision(6) << raiz
                                    << "  (Newton-Raphson, x0 = " << x0 << ", " << iters << " iteracoes)";
                                salvarRelatorio("Raiz (Newton-Raphson)", oss.str());
                            }
                        }
                        break;
                    }

                    // ──────── PONTOS CRÍTICOS (MÁX/MÍN) ────────
                    case 8: {
                        cout << "\n  ── Pontos Críticos (máximos, mínimos e inflexões) ──" << endl;
                        cout << "  Procura onde f'(x) = 0 num intervalo [a, b] e classifica pela troca de sinal de f'.\n" << endl;
                        double a, b;
                        cout << "  a (início do intervalo) = ";
                        a = lerValor();
                        cout << "  b (fim do intervalo) = ";
                        b = lerValor();
                        acharPontosCriticos(posfixo, a, b);
                        break;
                    }

                    case 0:
                        break; // Volta ao menu principal

                    default:
                        cout << "  Opção inválida." << endl;
                        break;
                }
                break; // Fim do case 1 (Cálculo 1)
            }

            // ============================================================
            //                       CÁLCULO 2
            // ============================================================
            case 2: {
                cout << "\n  ────────────────────────────────────────" << endl;
                cout << "              MENU CÁLCULO 2              " << endl;
                cout << "  ────────────────────────────────────────" << endl;
                cout << "  [1] Série de Taylor / Maclaurin" << endl;
                cout << "  [2] Equação Diferencial (Método de Euler)" << endl;
                cout << "  [3] Carregar coeficientes de Taylor (arquivo binário)" << endl;
                cout << "  [4] Derivadas Parciais (∂F/∂x, ∂F/∂y)" << endl;
                cout << "  [5] Teste de Convergência de Série" << endl;
                cout << "  [0] Voltar" << endl;
                cout << "  ────────────────────────────────────────" << endl;

                int op_c2; // Opção do submenu de Cálculo 2
                bool valido_c2;
                do {
                    cout << "  Opção: ";
                    op_c2 = lerInteiro();
                    valido_c2 = (op_c2 >= 0 && op_c2 <= 5);
                    if (!valido_c2) {
                        cout << "  Opção inválida! Escolha entre 0 e 5." << endl;
                    }
                } while (!valido_c2);

                switch (op_c2) {

                    // ──────── SÉRIE DE TAYLOR / MACLAURIN ────────
                    case 1: {
                        cout << "\n  ── Série de Potência ──" << endl;
                        cout << "  [1] Série de Taylor  (centro a genérico)" << endl;
                        cout << "  [2] Série de Maclaurin (centro a = 0)" << endl;

                        int tipoSerie; // Escolha entre Taylor e Maclaurin
                        cout << "  Opção: ";
                        tipoSerie = lerInteiro();

                        double a = 0.0; // Centro da série (padrão: 0 para Maclaurin)
                        if (tipoSerie == 1) {
                            cout << "  Centro a = ";
                            a = lerValor(); // Lê o centro para Taylor genérico
                        }

                        const int MAX_TERMOS = 30; // acima disso os fatoriais ficam ilegíveis
                        int numTermos; // Quantidade de termos do polinômio
                        do {
                            cout << "  Número de termos (1 a " << MAX_TERMOS << "): ";
                            numTermos = lerInteiro();
                            if (cin.eof()) break;
                        } while (numTermos <= 0 || numTermos > MAX_TERMOS);

                        // Calcula os coeficientes e formata o polinômio
                        vector<double> coefs = calcularCoefsTaylor(posfixo, a, numTermos);

                        // Sem derivadas finitas em a não existe polinômio de Taylor
                        bool derivavel = true;
                        for (size_t i = 0; i < coefs.size(); i++) {
                            if (isnan(coefs[i]) || isinf(coefs[i])) derivavel = false;
                        }
                        if (!derivavel) {
                            cout << "\n  Não é possível montar o polinômio: f ou uma de suas derivadas\n"
                                 << "  não está definida (ou é grande demais) em a = " << a << "." << endl;
                            break;
                        }
                        string polinomio = formatarSerieTaylor(coefs, a);

                        // Exibe o tipo da série e o polinômio
                        cout << "\n  ── " << (abs(a) < 1e-10 ? "Série de Maclaurin" : "Série de Taylor")
                             << " com " << numTermos << " termos ──" << endl;
                        cout << "  " << polinomio << endl;

                        // Pergunta se o usuário quer substituir um valor para x
                        char substituir; // Variável char para ler a resposta
                        cout << "\n  Deseja substituir um valor para x? (s/n): ";
                        cin >> substituir;

                        // Verifica se a resposta é 's' ou 'S' usando operador lógico OR (||)
                        if (substituir == 's' || substituir == 'S') {
                            double xVal; // Valor de x para substituição
                            cout << "  Valor de x = ";
                            xVal = lerValor();

                            double resultTaylor = avaliarSerieTaylor(coefs, a, xVal);   // P(x)
                            double resultExato = avaliar(posfixo, xVal);                // f(x)

                            cout << "\n  P(" << xVal << ") = " << fixed << setprecision(6)
                                 << resultTaylor << "  (aproximação pela série)" << endl;
                            cout << "  f(" << xVal << ") = " << resultExato
                                 << "  (valor exato da função)" << endl;

                            // Calcula o erro da aproximação
                            double erro = abs(resultExato - resultTaylor);
                            cout << "  Erro absoluto: " << erro << endl;
                        } else {
                            cout << "\n  Polinômio exibido sem substituição." << endl;
                        }

                        // Oferece salvar o polinômio em arquivo de texto
                        if (perguntarSim("\n  Salvar o polinômio em arquivo de texto? (s/n): ")) {
                            salvarRelatorio("Serie de Taylor/Maclaurin",
                                            "f(x) = " + expressao + "\n" + polinomio +
                                            "  (centro a = " + to_string(a) + ")");
                        }

                        // Oferece salvar os COEFICIENTES em arquivo binário (tópico IX)
                        if (perguntarSim("  Salvar os coeficientes em arquivo binário (coeficientes.bin)? (s/n): ")) {
                            if (salvarCoeficientesBin("coeficientes.bin", coefs, a))
                                cout << "  Coeficientes salvos em coeficientes.bin" << endl;
                            else
                                cout << "  Erro ao gravar o arquivo binário." << endl;
                        }
                        break;
                    }

                    // ──────── EQUAÇÃO DIFERENCIAL (EULER) ────────
                    case 2: {
                        cout << "\n  ── Equação Diferencial: Método de Euler ──" << endl;
                        cout << "  Resolve dy/dx = g(x, y) numericamente.\n" << endl;

                        string exprEDO;
                        vector<Token> posfixoEDO;

                        while (true) {
                            cout << "  Digite a função g(x, y) (ex: x + y, x*y, -2*y): ";
                            getline(cin, exprEDO); // Lê a expressão da EDO

                            // Processa a expressão da EDO
                            string erro;
                            if (processarExpressao(exprEDO, posfixoEDO, erro)) break;
                            cout << "  " << erro << "\n\n";
                        }

                        double x0, y0, xf; // Condições iniciais e ponto final
                        int numPassos;      // Número de passos de Euler

                        cout << "  Condição inicial x₀ = ";
                        x0 = lerValor();
                        cout << "  Condição inicial y(x₀) = ";
                        y0 = lerValor();
                        cout << "  Calcular até x = ";
                        xf = lerValor();
                        do {
                            cout << "  Número de passos (> 0): ";
                            numPassos = lerInteiro();
                        } while (numPassos <= 0); // valida quantidade positiva

                        cout << "\n  Resolvendo dy/dx = " << exprEDO
                             << "  com y(" << x0 << ") = " << y0 << endl;

                        // Pergunta se quer exportar a tabela completa para CSV
                        string csvEDO = "";
                        if (perguntarSim("  Exportar a tabela para CSV (tabela_euler.csv)? (s/n): "))
                            csvEDO = "tabela_euler.csv";

                        resolverEDOEuler(posfixoEDO, x0, y0, xf, numPassos, csvEDO); // Executa Euler
                        break;
                    }

                    // ──────── CARREGAR COEFICIENTES (ARQUIVO BINÁRIO) ────────
                    case 3: {
                        cout << "\n  ── Carregar Coeficientes de Taylor (arquivo binário) ──" << endl;
                        vector<double> coefs;
                        double centro = 0.0;
                        if (carregarCoeficientesBin("coeficientes.bin", coefs, centro)) {
                            cout << "  Lidos " << coefs.size()
                                 << " coeficientes (centro a = " << centro << ")." << endl;
                            cout << "  " << formatarSerieTaylor(coefs, centro) << endl;

                            if (perguntarSim("\n  Avaliar o polinômio em um valor de x? (s/n): ")) {
                                cout << "  Valor de x = ";
                                double xv = lerValor();
                                cout << "  P(" << xv << ") = " << fixed << setprecision(6)
                                     << avaliarSerieTaylor(coefs, centro, xv) << endl;
                            }
                        } else {
                            cout << "  Não foi possível ler 'coeficientes.bin'." << endl;
                            cout << "  (Gere o arquivo primeiro na opção [1], salvando os coeficientes.)" << endl;
                        }
                        break;
                    }

                    // ──────── DERIVADAS PARCIAIS ────────
                    case 4: {
                        cout << "\n  ── Derivadas Parciais ──" << endl;
                        cout << "  Para F(x, y), calcula ∂F/∂x e ∂F/∂y (a outra variável fica constante).\n" << endl;

                        string exprP;
                        vector<Token> posfixoP;
                        while (true) {
                            cout << "  Digite F(x, y) (ex: x^2*y + sin(x*y)): ";
                            getline(cin, exprP);
                            string erro;
                            if (processarExpressao(exprP, posfixoP, erro)) break;
                            cout << "  " << erro << "\n\n";
                        }

                        // Derivadas parciais SIMBÓLICAS (reaproveita diferenciar com a variável)
                        No* arvP = construirArvore(posfixoP);
                        No* dFx = diferenciar(arvP, 'x');
                        No* dFy = diferenciar(arvP, 'y');
                        for (int s = 0; s < 5; s++) { dFx = simplificar(dFx); dFy = simplificar(dFy); }
                        string dFxStr = noParaString(dFx);
                        string dFyStr = noParaString(dFy);

                        cout << "\n  ── Resultado Simbólico ──" << endl;
                        cout << "  F(x, y) = " << exprP << endl;
                        cout << "  ∂F/∂x   = " << dFxStr << endl;
                        cout << "  ∂F/∂y   = " << dFyStr << endl;

                        // Avaliação numérica em um ponto (usa a SOBRECARGA derivada(f,x,y,var))
                        if (perguntarSim("\n  Substituir um ponto (x, y)? (s/n): ")) {
                            cout << "  x = ";
                            double xp = lerValor();
                            cout << "  y = ";
                            double yp = lerValor();
                            cout << "\n  ── Valores Numéricos em (" << xp << ", " << yp << ") ──" << endl;
                            cout << "  ∂F/∂x = " << fixed << setprecision(6) << derivada(posfixoP, xp, yp, 'x') << endl;
                            cout << "  ∂F/∂y = " << derivada(posfixoP, xp, yp, 'y') << endl;
                        }

                        if (perguntarSim("\n  Salvar resultado em arquivo? (s/n): ")) {
                            salvarRelatorio("Derivadas Parciais",
                                            "F(x, y) = " + exprP + "\n∂F/∂x = " + dFxStr + "\n∂F/∂y = " + dFyStr);
                        }

                        liberarArvore(arvP);     // Libera F e as duas derivadas parciais
                        liberarArvore(dFx);
                        liberarArvore(dFy);
                        break;
                    }

                    // ──────── TESTE DE CONVERGÊNCIA DE SÉRIE ────────
                    case 5: {
                        cout << "\n  ── Teste de Convergência de Série  Σ a(n) ──" << endl;
                        cout << "  Usa a função atual como termo geral a(n), com n = x:" << endl;
                        cout << "  a(n) = " << expressao << "\n" << endl;

                        cout << "  (testes aplicados numericamente, para n grande)" << endl;

                        // 1) Teste do termo geral: a série só pode convergir se a(n) → 0
                        bool jaDecidiu = false;
                        if (termoNaoTendeAZero(posfixo)) {
                            cout << "  Teste do termo geral: |a(n)| não diminui em direção a 0." << endl;
                            cout << "  → A série DIVERGE." << endl;
                            jaDecidiu = true;
                        } else {
                            cout << "  Teste do termo geral: não indica divergência." << endl;
                        }

                        // 2) Teste da razão (d'Alembert)
                        if (!jaDecidiu) {
                            double L = testeRazaoSerie(posfixo);
                            if (isnan(L)) {
                                cout << "  Teste da razão: a razão oscila ou não pode ser calculada." << endl;
                            } else {
                                cout << "  Teste da razão: L = lim |a(n+1)/a(n)| ≈ "
                                     << fixed << setprecision(6) << L << endl;
                            }
                            if (!isnan(L) && L < 0.99)      { cout << "  → A série CONVERGE (L < 1)." << endl; jaDecidiu = true; }
                            else if (!isnan(L) && L > 1.01) { cout << "  → A série DIVERGE (L > 1)." << endl;  jaDecidiu = true; }
                            else                              cout << "  → Teste da razão INCONCLUSIVO." << endl;
                        }

                        // 3) Razão constante, Leibniz ou comparação com a série-p
                        if (!jaDecidiu) {
                            string motivo;
                            Veredito veredito = analisarDecaimento(posfixo, motivo);
                            cout << "  Análise do decaimento: " << motivo << endl;
                            if (veredito == SERIE_CONVERGE)     cout << "  → A série CONVERGE." << endl;
                            else if (veredito == SERIE_DIVERGE) cout << "  → A série DIVERGE." << endl;
                            else cout << "  → Resultado INCONCLUSIVO." << endl;
                        }

                        // Somas parciais apenas para conferência: sozinhas não provam nada
                        cout << "\n  Somas parciais (termos indefinidos são ignorados):" << endl;
                        cout << "  S(100) = " << fmtSoma(somaParcialSerie(posfixo, 100))
                             << " | S(2000) = " << fmtSoma(somaParcialSerie(posfixo, 2000))
                             << " | S(10000) = " << fmtSoma(somaParcialSerie(posfixo, 10000)) << endl;
                        break;
                    }

                    case 0:
                        break; // Volta ao menu principal

                    default:
                        cout << "  Opção inválida." << endl;
                        break;
                }
                break; // Fim do case 2 (Cálculo 2)
            }

            // ============================================================
            //                    ALTERAR FUNÇÃO
            // ============================================================
            case 3: {
                cout << "\n  Função anterior: f(x) = " << expressao << endl;

                while (true) {
                    cout << "  Nova função f(x): ";
                    getline(cin, expressao); // Lê a nova expressão

                    // Reprocessa a expressão; precisa ser uma função de x
                    string erro;
                    if (processarExpressao(expressao, posfixo, erro, "x")) break;
                    cout << "  " << erro << "\n\n";
                }

                cout << "  Função atualizada com sucesso!" << endl;
                break;
            }

            // ============================================================
            //                         SAIR
            // ============================================================
            case 0: {
                rodando = false; // Define a flag como false para encerrar o while
                cout << "\n  ══════════════════════════════════════════════════════" << endl;
                cout << "        Encerrando o programa. Bons estudos!           " << endl;
                cout << "  ══════════════════════════════════════════════════════\n" << endl;
                break;
            }

            default:
                cout << "  Opção inválida. Tente novamente." << endl;
                break;
        }
    }

    return 0;
}
