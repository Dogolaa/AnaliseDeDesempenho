#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<stdbool.h>

#define NUM_FILAS 3

/*
 * ===================================================================
 * ESTRUTURAS DE DADOS
 * ===================================================================
 * Requisicao: Representa um único cliente, armazenando seu tempo de chegada.
 * No: Componente da lista ligada que representa a fila. Contém uma requisição
 * e um ponteiro para o próximo nó.
 * medida_little: Estrutura para auxiliar nos cálculos da Lei de Little,
 * armazenando a área acumulada ao longo do tempo.
 */
typedef struct {
    double tempo_chegada;
} Requisicao;

typedef struct No {
    Requisicao req;
    struct No* proximo;
} No;

typedef struct {
    double tempo_anterior;
    unsigned long int qt_requisicoes;
    double soma_area;
} medida_little;

/*
 * ===================================================================
 * FUNÇÕES AUXILIARES
 * ===================================================================
 * aleatorio(): Gera um número aleatório no intervalo (0, 1].
 * exponencial(l): Gera um número aleatório a partir de uma distribuição
 * exponencial com taxa 'l' (lambda). Usado para simular
 * os intervalos entre chegadas e os tempos de serviço.
 * inicia_little(): Inicializa os campos de uma estrutura medida_little.
 */
double aleatorio() {
    double u = rand() / ((double) RAND_MAX + 1);
    u = 1.0 - u;
    return (u);
}

double exponencial(double l){
    return (-1.0/l)*log(aleatorio());
}

void inicia_little(medida_little * medidas){
    medidas->tempo_anterior = 0.0;
    medidas->qt_requisicoes = 0;
    medidas->soma_area = 0.0;
}

int main(void){
    srand(time(NULL));

    /*
     * ===================================================================
     * INICIALIZAÇÃO DE VARIÁVEIS DA SIMULAÇÃO
     * ===================================================================
     * - Métricas de Little (E[N], E[W]).
     * - Variáveis de controle do tempo e do estado do servidor.
     * - Estruturas para as múltiplas filas (cabeça, cauda, tamanho).
     * - Contadores para estatísticas gerais (perdas, chegadas, etc.).
     */
    medida_little E_N;
    medida_little E_W_chegadas;
    medida_little E_W_saidas;
    inicia_little(&E_N);
    inicia_little(&E_W_chegadas);
    inicia_little(&E_W_saidas);
    
    double tempo_decorrido = 0.0;
    double tempo_simulacao = 86400.0;
    bool servidor_ocupado = false;

    double media_inter_requisicoes[NUM_FILAS];
    double proxima_requisicao[NUM_FILAS];
    
    No* cabeca_fila[NUM_FILAS] = {NULL};
    No* cauda_fila[NUM_FILAS] = {NULL};
    unsigned long int tamanho_fila[NUM_FILAS] = {0};
    unsigned long int max_fila;
    unsigned long int perdas[NUM_FILAS] = {0};

    double media_tempo_servico;
    double tempo_saida_servico = tempo_simulacao * 2;

    unsigned long int total_chegadas = 0;
    unsigned long int total_servicos_completos = 0;
    double soma_tempo_servico = 0.0;
    
    /*
     * ===================================================================
     * COLETA DE PARÂMETROS DE ENTRADA
     * ===================================================================
     * Solicita ao usuário as taxas de chegada para cada uma das 3 filas,
     * a taxa de atendimento do servidor e o tamanho máximo das filas.
     */
    printf("---=== Simulador de Fila Única com Múltiplas Filas ===---\n");
    for (int i = 0; i < NUM_FILAS; i++) {
        printf("Informe a taxa de chegada da Fila %d (reqs/segundo): ", i + 1);
        scanf("%lf", &media_inter_requisicoes[i]);
    }
    printf("Informe a taxa de atendimento do servidor (reqs/segundo): ");
    scanf("%lf", &media_tempo_servico);
    printf("Informe o tamanho maximo para cada fila: ");
    scanf("%lu", &max_fila);

    /*
     * ===================================================================
     * SETUP INICIAL DA SIMULAÇÃO
     * ===================================================================
     * - Agenda a primeira chegada para cada uma das filas.
     * - Abre o arquivo CSV de saída e escreve o cabeçalho.
     * - Agenda o primeiro evento de relatório.
     */
    for (int i = 0; i < NUM_FILAS; i++) {
        proxima_requisicao[i] = exponencial(media_inter_requisicoes[i]);
    }
    
    FILE *arquivo_saida = fopen("relatorio_simulacao.csv", "w");
    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saida!\n");
        return 1; 
    }
    fprintf(arquivo_saida, "Tempo(s),Fila1,Fila2,Fila3,TotalSistema,ServidorOcupado,E[N],E[W]\n");
    double proximo_ponto_relatorio = 10.0;

    /*
     * ===================================================================
     * LAÇO PRINCIPAL DA SIMULAÇÃO (MOTOR DE EVENTOS)
     * ===================================================================
     * O laço continua enquanto o tempo decorrido for menor que o tempo
     * total da simulação. A cada iteração, ele determina o próximo
     * evento, avança o tempo e processa o evento correspondente.
     */
    while(tempo_decorrido < tempo_simulacao){
        
        /*
         * Bloco 1: DETERMINAÇÃO DO PRÓXIMO EVENTO
         * Compara o tempo agendado para todas as possíveis chegadas (uma
         * para cada fila), a saída do servidor (se ocupado) e o próximo
         * relatório. O evento com o menor tempo é selecionado para ser
         * o próximo a ocorrer. A variável 'tipo_evento' armazena qual
         * evento foi escolhido.
         */
        double tempo_proximo_evento = tempo_simulacao * 2;
        int tipo_evento = -1;

        for (int i = 0; i < NUM_FILAS; i++) {
            if (proxima_requisicao[i] < tempo_proximo_evento) {
                tempo_proximo_evento = proxima_requisicao[i];
                tipo_evento = i;
            }
        }
        
        if (servidor_ocupado && tempo_saida_servico < tempo_proximo_evento) {
            tempo_proximo_evento = tempo_saida_servico;
            tipo_evento = 3;
        }
        
        if (proximo_ponto_relatorio < tempo_proximo_evento) {
            tempo_proximo_evento = proximo_ponto_relatorio;
            tipo_evento = 4;
        }

        /*
         * Bloco 2: AVANÇO DO TEMPO E ATUALIZAÇÃO DE MÉTRICAS
         * O relógio da simulação "salta" para o tempo do próximo evento.
         * Antes de alterar o estado do sistema, as áreas para o cálculo
         * de Little são atualizadas com base no tempo que passou desde
         * o último evento.
         */
        tempo_decorrido = tempo_proximo_evento;

        if (tempo_decorrido > tempo_simulacao) {
            break;
        }

        double delta_t = tempo_decorrido - E_N.tempo_anterior;
        E_N.soma_area += delta_t * E_N.qt_requisicoes;
        E_W_chegadas.soma_area += delta_t * E_W_chegadas.qt_requisicoes;
        E_W_saidas.soma_area += delta_t * E_W_saidas.qt_requisicoes;
        E_N.tempo_anterior = tempo_decorrido;
        E_W_chegadas.tempo_anterior = tempo_decorrido;
        E_W_saidas.tempo_anterior = tempo_decorrido;

        /*
         * ===================================================================
         * Bloco 3: PROCESSAMENTO DE EVENTOS
         * ===================================================================
         * Com base no 'tipo_evento' determinado, o estado do sistema é
         * modificado.
         */

        /*
         * Evento de CHEGADA (tipo_evento de 0 a NUM_FILAS-1)
         * - Verifica se a fila correspondente tem espaço.
         * - Se sim, cria uma nova requisição, a adiciona no fim da fila
         * e atualiza as estatísticas.
         * - Se o servidor estava ocioso, o serviço já é iniciado.
         * - Se não, o cliente é contabilizado como uma perda.
         * - Por fim, agenda a próxima chegada para ESTA fila.
         */
        if(tipo_evento >= 0 && tipo_evento < NUM_FILAS) {
            int fila_idx = tipo_evento;

            if (tamanho_fila[fila_idx] < max_fila) {
                No* novo_no = (No*) malloc(sizeof(No));
                novo_no->req.tempo_chegada = tempo_decorrido;
                novo_no->proximo = NULL;

                if (cabeca_fila[fila_idx] == NULL) {
                    cabeca_fila[fila_idx] = novo_no;
                    cauda_fila[fila_idx] = novo_no;
                } else {
                    cauda_fila[fila_idx]->proximo = novo_no;
                    cauda_fila[fila_idx] = novo_no;
                }
                tamanho_fila[fila_idx]++;
                total_chegadas++;

                E_N.qt_requisicoes++;
                E_W_chegadas.qt_requisicoes++;

                if (!servidor_ocupado) {
                    No* no_atendido = cabeca_fila[fila_idx];
                    cabeca_fila[fila_idx] = no_atendido->proximo;
                    if (cabeca_fila[fila_idx] == NULL) cauda_fila[fila_idx] = NULL;
                    tamanho_fila[fila_idx]--;
                    free(no_atendido);
                    
                    double duracao_servico = exponencial(media_tempo_servico);
                    tempo_saida_servico = tempo_decorrido + duracao_servico;
                    soma_tempo_servico += duracao_servico;
                    servidor_ocupado = true;
                }
            } else {
                perdas[fila_idx]++;
            }
            
            proxima_requisicao[fila_idx] = tempo_decorrido + exponencial(media_inter_requisicoes[fila_idx]);

        /*
         * Evento de SAÍDA (tipo_evento == 3)
         * O servidor terminou de atender um cliente.
         * - Atualiza as estatísticas de saída.
         * - Executa a LÓGICA DE DECISÃO: verifica o primeiro cliente de
         * todas as filas não-vazias e escolhe aquele com o menor
         * tempo de chegada (o mais antigo no sistema).
         * - Se um cliente é escolhido, ele é removido de sua fila e um
         * novo tempo de serviço é agendado.
         * - Se não há ninguém em nenhuma fila, o servidor fica ocioso.
         */
        } else if (tipo_evento == 3) {
            total_servicos_completos++;
            E_N.qt_requisicoes--;
            E_W_saidas.qt_requisicoes++;
            
            int fila_a_servir = -1;
            double menor_tempo_chegada = tempo_simulacao * 2;

            for (int i = 0; i < NUM_FILAS; i++) {
                if (cabeca_fila[i] != NULL) {
                    if (cabeca_fila[i]->req.tempo_chegada < menor_tempo_chegada) {
                        menor_tempo_chegada = cabeca_fila[i]->req.tempo_chegada;
                        fila_a_servir = i;
                    }
                }
            }

            if (fila_a_servir != -1) {
                No* no_atendido = cabeca_fila[fila_a_servir];
                cabeca_fila[fila_a_servir] = no_atendido->proximo;
                if (cabeca_fila[fila_a_servir] == NULL) {
                    cauda_fila[fila_a_servir] = NULL;
                }
                tamanho_fila[fila_a_servir]--;
                free(no_atendido);
                
                double duracao_servico = exponencial(media_tempo_servico);
                tempo_saida_servico = tempo_decorrido + duracao_servico;
                soma_tempo_servico += duracao_servico;
                servidor_ocupado = true;
            } else {
                servidor_ocupado = false;
                tempo_saida_servico = tempo_simulacao * 2;
            }

        /*
         * Evento de RELATÓRIO (tipo_evento == 4)
         * - Calcula as métricas de desempenho parciais (E[N] e E[W]).
         * - Grava uma nova linha no arquivo CSV com o estado atual.
         * - Agenda o próximo evento de relatório.
         */
        } else if (tipo_evento == 4) {
            double E_N_atual = E_N.soma_area / tempo_decorrido;
            double E_W_atual = 0.0;
            if (E_W_chegadas.qt_requisicoes > 0) {
                E_W_atual = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
            }

            fprintf(arquivo_saida, "%.0f,%lu,%lu,%lu,%lu,%d,%f,%f\n", 
                    tempo_decorrido, 
                    tamanho_fila[0], tamanho_fila[1], tamanho_fila[2], 
                    E_N.qt_requisicoes, 
                    servidor_ocupado, E_N_atual, E_W_atual);
            fflush(arquivo_saida);

            proximo_ponto_relatorio += 10.0;
        }
    }

    /*
     * ===================================================================
     * FINALIZAÇÃO E APRESENTAÇÃO DOS RESULTADOS
     * ===================================================================
     * - Faz uma última atualização das áreas de Little para o tempo exato
     * de fim da simulação.
     * - Imprime no console um resumo com as estatísticas finais: totais de
     * chegadas, serviços, perdas, ocupação do servidor e os resultados
     * finais da Lei de Little (E[N], E[W]).
     * - Fecha o arquivo e libera a memória alocada para os nós restantes
     * nas filas (se houver).
     */
    tempo_decorrido = tempo_simulacao;
    double delta_t = tempo_decorrido - E_N.tempo_anterior;
    E_N.soma_area += delta_t * E_N.qt_requisicoes;
    E_W_chegadas.soma_area += delta_t * E_W_chegadas.qt_requisicoes;
    E_W_saidas.soma_area += delta_t * E_W_saidas.qt_requisicoes;

    printf("\n---=== Simulacao Finalizada ===---\n");
    printf("Tempo total de simulacao: %.2f segundos\n", tempo_decorrido);
    printf("Total de chegadas ao sistema: %lu\n", total_chegadas);
    printf("Total de servicos completos: %lu\n", total_servicos_completos);
    for (int i = 0; i < NUM_FILAS; i++) {
        printf("Clientes perdidos na Fila %d (fila cheia): %lu\n", i + 1, perdas[i]);
    }

    printf("\n---=== Metricas de Desempenho ===---\n");
    printf("Ocupacao calculada do servidor: %f\n", soma_tempo_servico / tempo_decorrido);
        
    printf("\n---=== Lei de Little ===---\n");
    double E_N_final = E_N.soma_area / tempo_decorrido;
    double lambda_efetivo = (double)E_W_chegadas.qt_requisicoes / tempo_decorrido;
    double E_W_final = 0.0;
    if (E_W_chegadas.qt_requisicoes > 0){
        E_W_final = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
    }
    double erro_little = E_N_final - lambda_efetivo * E_W_final;  

    printf("E[N] (numero medio de clientes no sistema): %f\n", E_N_final);
    printf("E[W] (tempo medio do cliente no sistema): %f\n", E_W_final);
    printf("Lambda Efetivo (taxa de chegada real): %f\n", lambda_efetivo);
    printf("Erro numerico (Little): %e\n", erro_little);

    fclose(arquivo_saida);
    
    for(int i = 0; i < NUM_FILAS; i++){
        No* atual = cabeca_fila[i];
        while(atual != NULL){
            No* temp = atual;
            atual = atual->proximo;
            free(temp);
        }
    }

    return 0;
}