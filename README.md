# Calculadora de Cálculo 1 e 2 em C++

Calculadora interativa de terminal para Cálculo 1 e 2, com parser de expressões próprio, derivação e integração simbólicas e métodos numéricos.

## Sobre o projeto

Este é um **projeto antigo**: foi meu projeto final da disciplina de Computação para Engenharia (CPE), feito no meio de 2026. A versão original do código é de 30/06/2026. **Só lembrei de colocá-lo no GitHub recentemente**, em novembro de 2026.

Antes de publicar, o código passou por uma revisão com correções e uma funcionalidade nova, descritas em [Alterações desde julho](#alterações-desde-julho).

Autor: Arthur Lima Rocha Ribeiro Paes

## Como compilar e executar

O programa usa `windows.h` para exibir acentos e símbolos no console, portanto roda no **Windows**. Foi testado com o MinGW (GCC 6.3.0).

```powershell
g++ calculadora.cpp algebra.cpp -o calculadora.exe
```

```powershell
.\calculadora.exe
```

## O que a calculadora faz

Primeiro você digita uma função `f(x)`. Depois escolhe a operação nos menus.

| Cálculo 1 | Cálculo 2 |
|---|---|
| 1. Limite (com a derivada simbólica) | 1. Série de Taylor / Maclaurin |
| 2. Continuidade em um ponto | 2. Equação diferencial (método de Euler) |
| 3. Derivadas primeira e segunda | 3. Carregar coeficientes de Taylor (arquivo binário) |
| 4. Derivada implícita | 4. Derivadas parciais |
| 5. Integral indefinida | 5. Teste de convergência de série |
| 6. Integral definida | |
| 7. Raízes (Newton-Raphson) | |
| 8. Pontos críticos (máximos e mínimos) | |

## Como digitar expressões

| Item | Aceito |
|---|---|
| Operadores | `+` `-` `*` `/` `^` e parênteses |
| Funções | `sin` `cos` `tan` `asin` `acos` `atan` `sinh` `cosh` `tanh` `sec` `csc` `cot` `ln` `log` (base 10) `exp` `sqrt` `abs` |
| Constantes | `pi` e `e` |
| Variáveis | `x`; `y` na derivada implícita, nas derivadas parciais e na equação diferencial |
| Multiplicação implícita | `2x`, `2pi`, `3sin(x)`, `(x+1)(x-1)` |

Regras importantes:

- Funções precisam de parênteses: `sin(x)`, não `sin x`.
- Use `sin` e `tan`, não `sen` e `tg`.
- O separador decimal é o ponto: `2.5`.
- Entre variáveis, escreva a multiplicação: `x*y`.
- A potência vem antes do sinal: `-x^2` significa `-(x²)`.
- Os campos numéricos aceitam expressões como `pi/2`, `sqrt(2)` e `1e-3`, um valor por linha.

## Exemplos de saída

**Pontos críticos** de `x^3 - 3x` em [-3, 3]:

```
x = -1.000000  | f(x) = 2.000000  → MÁXIMO local
x = 1.000000  | f(x) = -2.000000  → MÍNIMO local
```

**Integral** de `sin(x^2)` de 0 a 1, usando série de Taylor:

```
F(x) ≈ C + 0.333333x³ - 0.0238095x⁷ + 0.000757576x¹¹ - ...
∫[0, 1] f(x) dx ≈ 0.31026830  (termos até o grau 27)
Conferência pela Regra de Simpson: 0.31026830
```

**Convergência** da série de `1/x^1.5`:

```
Teste do termo geral: não indica divergência.
Teste da razão: L = lim |a(n+1)/a(n)| ≈ 0.998128
→ Teste da razão INCONCLUSIVO.
Análise do decaimento: |a(n)| se comporta como 1/n^p, com p ≈ 1.500 > 1.
→ A série CONVERGE.
```

## Estrutura do código

| Arquivo | Conteúdo |
|---|---|
| [`algebra.h`](algebra.h) | Declarações: tipos e protótipos da biblioteca matemática |
| [`algebra.cpp`](algebra.cpp) | Implementação: parser, métodos numéricos e cálculo simbólico |
| [`calculadora.cpp`](calculadora.cpp) | Programa principal: menus, leitura de entrada e relatórios |

### Do texto ao número

A expressão digitada passa por estas etapas:

1. `tokenizar` divide o texto em *tokens* (números, variáveis, operadores, funções). O menos unário vira um operador próprio, diferente da subtração.
2. `inserirMultImplicita` transforma `3x` em `3*x`.
3. `validarSintaxe` percorre os tokens alternando entre "espero um valor" e "espero um operador", e devolve uma mensagem se algo estiver errado.
4. `paraPosfixo` converte para a notação pós-fixa com o algoritmo *Shunting-yard*, de Dijkstra.
5. `avaliar` calcula o valor com uma pilha.

Por exemplo, `3*x^2 + sin(x)` vira `3 x 2 ^ * x sin +`.

### Cálculo simbólico

A pós-fixa também pode virar uma árvore (`struct No`), usada por:

- `diferenciar`: regras da soma, produto, quociente, potência e cadeia.
- `integrar`: tabela de primitivas, substituição, integração por partes e frações parciais com denominador de grau até 2. Retorna `nullptr` quando não encontra uma primitiva.
- `simplificar`: reduz expressões como `x + 0` e `1*x`.
- `noParaString`: converte a árvore de volta para texto.

### Série de Taylor

`calcularCoefsTaylor` usa **aritmética de séries**: em vez de aproximar derivadas, cada operação da expressão é aplicada a listas de coeficientes. O produto de duas séries, por exemplo, é `r[k] = Σ u[j]·v[k-j]`. Existem regras equivalentes para divisão, potência, exponencial, logaritmo e funções trigonométricas.

### Integrais

- `integralDefinida` aplica a Regra de Simpson 1/3.
- `integrarNumericamente` procura antes as singularidades no intervalo. Para cada uma, estima o expoente `p` em `|f| ~ 1/h^p` para saber se a integral diverge. Quando ela converge, usa a mudança de variável `x = s ± e^v` perto do ponto `s`.
- `integrarPorTaylor` é usada quando não há primitiva elementar conhecida e o usuário aceita. Integra a série termo a termo, divide o intervalo em até 64 partes se necessário e confere a série contra a função. O resultado é comparado com Simpson.

### Pontos críticos

`acharPontosCriticos` procura onde `f'(x) = 0` ou não existe, descarta descontinuidades e classifica cada ponto pela troca de sinal de `f'`:

- de negativo para positivo: mínimo;
- de positivo para negativo: máximo;
- sem troca: inflexão com tangente horizontal.

### Convergência de séries

Os testes são aplicados nesta ordem, até um deles decidir:

1. Termo geral: `|a(n)|` diminui quando `n` cresce?
2. Teste da razão (d'Alembert).
3. Série geométrica, critério de Leibniz ou comparação com a série-p.

Se nenhum for seguro, o resultado é **inconclusivo**. As somas parciais aparecem apenas para conferência.

### Arquivos gerados pelo programa

| Arquivo | Conteúdo |
|---|---|
| `resultados.txt` | Relatórios em texto, acrescentados ao final |
| `tabela_integral.csv` | Tabela da integral indefinida |
| `tabela_euler.csv` | Todos os passos do método de Euler |
| `coeficientes.bin` | Binário: quantidade (`int`), centro (`double`) e coeficientes (`double`) |

## Alterações desde julho

O código original não foi modificado entre julho e novembro. Todas as mudanças abaixo são de **26/11/2026**.

### Correções

| Situação | Versão de julho | Versão atual |
|---|---|---|
| `-2^2` | 4 | -4 |
| `e^-x` | calculado como `(e^-1)·x` | `e^(-x)` |
| `2pi` | valia π | vale 2π |
| `2,5*x` ou `sen(x)` | resultado errado, sem aviso | mensagem de erro |
| Taylor de `e^x`, derivada de ordem 13 | -1.535.248 | 1 |
| Série de `10000000/x^2` | diverge | converge |
| Série de `1/(x*ln(x))` | sugeria convergência | inconclusivo |
| ∫ `1/x^2` de -1 a 1 | cerca de 2·10²⁷ | diverge |
| ∫ `sin(x)/x` de 0 a 1 | sem valor (NaN) | 0,946083 |
| Pontos críticos de `x^4` | inflexão | mínimo |
| Pontos críticos de `x^3` | nenhum | inflexão |
| Pontos críticos de `1/x^2` | máximo em 0 | nenhum |

Detalhes:

- **Parser e entrada:** o menos unário passou a respeitar a precedência. A validação recusa caracteres e nomes desconhecidos, números mal formados, valores sem operador entre eles e parênteses vazios. Campos numéricos repetem a pergunta quando a entrada é inválida.
- **Taylor:** as diferenças finitas recursivas foram substituídas pela aritmética de séries. O menu aceita de 1 a 30 termos e avisa quando o polinômio não existe.
- **Séries:** o limite fixo de `10⁻⁶` e o veredito pelas somas parciais deram lugar aos testes descritos acima.
- **Integrais:** passaram a identificar divergência, singularidades e pontos fora do domínio. A tabela escolhe outra referência quando não dá para integrar a partir de 0.
- **Pontos críticos:** a classificação pela segunda derivada numérica foi substituída pelo teste da primeira derivada.

### Funcionalidade nova

- Cálculo de integrais por **série de Taylor** quando o programa não encontra uma primitiva em funções elementares, como em `sin(x^2)` e `e^(-x^2)`.

### Organização

- `kaio.cpp` foi renomeado para `calculadora.cpp`.
- As funções `derivadaN` e `limiteInfinito` ficaram sem uso e foram removidas.
- Foi adicionado um `.gitignore`.

As alterações foram conferidas com programas de teste temporários, que não fazem parte deste repositório.

## Limitações conhecidas

- Os resultados numéricos são estimativas. Testes de séries e de divergência podem ficar inconclusivos ou falhar em casos incomuns.
- Em picos muito estreitos, uma integral marcada como "precisão limitada" pode ter erro de alguns por cento.
- A integral de `e^(1/x)` de 0 a 1 é informada como "grande demais", e não como divergente.
- Os limites laterais são aproximados e podem errar: `100/(x+0.001)` em 0 aparece como `+∞` pela direita, mas vale 100000.
- Em `sin(x)/x`, a descontinuidade em 0 não é identificada como removível.
- A integração simbólica não encontra algumas primitivas simples, como a de `(x+1)(x-1)`. A de `1/x` aparece como `ln(x)`, sem o módulo.
- Algumas expressões simbólicas saem pouco simplificadas, e potências encadeadas aparecem sem parênteses (`x²²`).
- `f(x)` aceita `y` e o trata como 0.
- `simplificar` não libera todos os nós descartados da memória.
