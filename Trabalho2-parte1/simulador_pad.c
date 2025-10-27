#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<stdbool.h>

#define NUM_FILAS 3

/*
 * ===================================================================
 * ESTRUTURAS DE DADOS (Sem alterações)
 * ===================================================================
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
 * FUNÇÕES AUXILIARES (Sem alterações)
 * ===================================================================
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
     * NOVAS VARIÁVEIS PARA O CÁLCULO DO ATRASO MÉDIO (PAD)
     * ===================================================================
     */
    double deltas[NUM_FILAS]; // Pesos (multiplicadores) de cada fila
    unsigned long int T_j[NUM_FILAS] = {0}; // Total de chegadas na janela
    double S_j[NUM_FILAS] = {0.0}; // Soma dos tempos de chegada dos que ESTÃO na fila
    double D_j_dep[NUM_FILAS] = {0.0}; // Soma dos atrasos dos que JÁ SAÍRAM

    // Variáveis de estado para o servidor
    double tempo_chegada_em_atendimento;
    int fila_em_atendimento = -1;
    
    /*
     * ===================================================================
     * COLETA DE PARÂMETROS DE ENTRADA
     * ===================================================================
     */
    printf("---=== Simulador com Politica PAD (Proportional Average Delay) ===---\n");
    for (int i = 0; i < NUM_FILAS; i++) {
        printf("Informe a taxa de chegada da Fila %d (reqs/segundo): ", i + 1);
        scanf("%lf", &media_inter_requisicoes[i]);
    }
    printf("Informe a taxa de atendimento do servidor (reqs/segundo): ");
    scanf("%lf", &media_tempo_servico);
    printf("Informe o tamanho maximo para cada fila: ");
    scanf("%lu", &max_fila);

    // Coleta dos novos pesos (deltas)
    printf("\n--- Informe os pesos (deltas) para cada fila ---\n");
    for (int i = 0; i < NUM_FILAS; i++) {
        printf("Informe o delta (peso) da Fila %d: ", i + 1);
        scanf("%lf", &deltas[i]);
    }


    /*
     * ===================================================================
     * SETUP INICIAL DA SIMULAÇÃO
     * ===================================================================
     */
    for (int i = 0; i < NUM_FILAS; i++) {
        proxima_requisicao[i] = exponencial(media_inter_requisicoes[i]);
    }
    
    FILE *arquivo_saida = fopen("relatorio_pad.csv", "w");
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
     */
    while(tempo_decorrido < tempo_simulacao){
        
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
         */

        /*
         * Evento de CHEGADA (tipo_evento de 0 a NUM_FILAS-1)
         * - Apenas adiciona o cliente na fila e atualiza as estatísticas
         * do PAD. O escalonador será chamado DEPOIS, se o servidor
         * estiver livre.
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

                // ATUALIZA ESTATÍSTICAS PAD (CHEGADA)
                T_j[fila_idx]++; // Incrementa o total de itens da janela [cite: 16, 42]
                S_j[fila_idx] += tempo_decorrido; // Soma o tempo de chegada [cite: 16, 42]

            } else {
                perdas[fila_idx]++;
            }
            
            proxima_requisicao[fila_idx] = tempo_decorrido + exponencial(media_inter_requisicoes[fila_idx]);

        /*
         * Evento de SAÍDA (tipo_evento == 3)
         * - Atualiza as estatísticas PAD do cliente que ACABOU de ser
         * atendido e libera o servidor.
         */
        } else if (tipo_evento == 3) {
            total_servicos_completos++;
            E_N.qt_requisicoes--;
            E_W_saidas.qt_requisicoes++;

            // ATUALIZA ESTATÍSTICAS PAD (SAÍDA)
            int fila_que_saiu = fila_em_atendimento;
            double atraso = tempo_decorrido - tempo_chegada_em_atendimento;
            D_j_dep[fila_que_saiu] += atraso; // Soma o atraso do cliente que saiu [cite: 69, 150]
            
            servidor_ocupado = false;
            fila_em_atendimento = -1;

        /*
         * Evento de RELATÓRIO (tipo_evento == 4)
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

        /*
         * ===================================================================
         * Bloco 4: ESCALONADOR (Chamado se o servidor está livre)
         * ===================================================================
         * Após qualquer evento, se o servidor estiver ocioso, ele tenta
         * puxar um novo cliente usando a política PAD.
         */
        if (!servidor_ocupado) {
            int fila_a_servir = -1;
            double max_prioridade = -1.0; // Usamos -1.0 pois prioridade (atraso) pode ser 0

            // Itera em todas as filas para calcular a prioridade
            for (int i = 0; i < NUM_FILAS; i++) {
                if (tamanho_fila[i] > 0) { // Só podemos servir filas não-vazias
                    
                    // Coleta as variáveis para a fórmula
                    double n = (double)tamanho_fila[i];
                    double t = tempo_decorrido;
                    double S = S_j[i];
                    double D_dep = D_j_dep[i];
                    double T = (double)T_j[i];
                    double delta_val = deltas[i];
                    
                    double atraso_medio = 0.0;
                    if (T > 0) {
                        // Aplica a fórmula do atraso médio 
                        atraso_medio = (n * t - S + D_dep) / T;
                    }

                    // A prioridade é o atraso médio ponderado
                    double prioridade = delta_val * atraso_medio;

                    if (prioridade > max_prioridade) {
                        max_prioridade = prioridade;
                        fila_a_servir = i;
                    }
                }
            }

            // Se o escalonador escolheu alguém...
            if (fila_a_servir != -1) {
                // Remove o cliente da cabeça da fila escolhida
                No* no_atendido = cabeca_fila[fila_a_servir];
                
                // Salva os dados do cliente para a estatística de SAÍDA
                tempo_chegada_em_atendimento = no_atendido->req.tempo_chegada;
                fila_em_atendimento = fila_a_servir;

                // ATUALIZA ESTATÍSTICAS PAD (INÍCIO DO SERVIÇO)
                // Remove o tempo de chegada do cliente da soma S_j
                S_j[fila_a_servir] -= tempo_chegada_em_atendimento;

                // Remove o nó da lista ligada
                cabeca_fila[fila_a_servir] = no_atendido->proximo;
                if (cabeca_fila[fila_a_servir] == NULL) {
                    cauda_fila[fila_a_servir] = NULL;
                }
                tamanho_fila[fila_a_servir]--;
                free(no_atendido);
                
                // Agenda a SAÍDA (fim do serviço)
                double duracao_servico = exponencial(media_tempo_servico);
                tempo_saida_servico = tempo_decorrido + duracao_servico;
                soma_tempo_servico += duracao_servico;
                servidor_ocupado = true;
            } else {
                // Nenhuma fila tem clientes, servidor fica ocioso
                servidor_ocupado = false;
                tempo_saida_servico = tempo_simulacao * 2;
            }
        }
    }

    /*
     * ===================================================================
     * FINALIZAÇÃO E APRESENTAÇÃO DOS RESULTADOS
     * ===================================================================
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