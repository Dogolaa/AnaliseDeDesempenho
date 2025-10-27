#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<stdbool.h>
#include<string.h> // Adicionado para strcpy e sprintf

#define NUM_FILAS 3
#define NUM_RUNS 4 // Quantidade de simulações (80, 90, 95, 99.9)

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
     * PARÂMETROS GLOBAIS DA SIMULAÇÃO
     * ===================================================================
     */
    double ocupacoes[NUM_RUNS] = {0.80, 0.90, 0.95, 0.999};
    char* labels[NUM_RUNS] = {"80", "90", "95", "99_9"};
    
    double deltas[NUM_FILAS] = {1.0, 2.0, 4.0};
    unsigned long int max_fila = 1000;
    double media_tempo_servico = 10.0; // 10 reqs/segundo
    double tempo_simulacao = 86400.0;
    
    printf("---=== Iniciando Bateria de Simulacoes (Politica PAD) ===---\n");
    printf("Taxa de Servico (u): %.2f | Max Fila: %lu\n", media_tempo_servico, max_fila);
    printf("Deltas: F1=%.1f, F2=%.1f, F3=%.1f\n", deltas[0], deltas[1], deltas[2]);

    /*
     * ===================================================================
     * LAÇO PRINCIPAL DAS SIMULAÇÕES (executa uma vez por ocupação)
     * ===================================================================
     */
    for (int run = 0; run < NUM_RUNS; run++) {

        /*
         * ===================================================================
         * Bloco 1: RESET DE TODAS AS VARIÁVEIS DE ESTADO
         * ===================================================================
         */
        medida_little E_N;
        medida_little E_W_chegadas;
        medida_little E_W_saidas;
        inicia_little(&E_N);
        inicia_little(&E_W_chegadas);
        inicia_little(&E_W_saidas);
        
        double tempo_decorrido = 0.0;
        bool servidor_ocupado = false;

        double media_inter_requisicoes[NUM_FILAS];
        double proxima_requisicao[NUM_FILAS];
        
        No* cabeca_fila[NUM_FILAS] = {NULL};
        No* cauda_fila[NUM_FILAS] = {NULL};
        unsigned long int tamanho_fila[NUM_FILAS] = {0};
        unsigned long int perdas[NUM_FILAS] = {0};

        double tempo_saida_servico = tempo_simulacao * 2;

        unsigned long int total_chegadas = 0;
        unsigned long int total_servicos_completos = 0;
        double soma_tempo_servico = 0.0;

        unsigned long int T_j[NUM_FILAS] = {0};
        double S_j[NUM_FILAS] = {0.0};
        double D_j_dep[NUM_FILAS] = {0.0};

        double tempo_chegada_em_atendimento = 0.0;
        int fila_em_atendimento = -1;
        double proximo_ponto_relatorio = 10.0;

        /*
         * ===================================================================
         * Bloco 2: CONFIGURAÇÃO DOS PARÂMETROS DA RODADA
         * ===================================================================
         */
        char nome_arquivo[100];
        double ocupacao_atual = ocupacoes[run];
        char* rho_label = labels[run];
        double lambda_total = media_tempo_servico * ocupacao_atual;

        media_inter_requisicoes[0] = lambda_total * (5.0 / 8.0);
        media_inter_requisicoes[1] = lambda_total * (2.0 / 8.0);
        media_inter_requisicoes[2] = lambda_total * (1.0 / 8.0);
        
        sprintf(nome_arquivo, "relatorio_pad_ocupacao_%s_pct.csv", rho_label);

        printf("\n---=== [RUN %d/%d] Iniciando: Ocupacao %s%% ===---\n", run + 1, NUM_RUNS, rho_label);
        printf("   Arquivo: %s | Lambda Total: %.4f\n", nome_arquivo, lambda_total);

        for (int i = 0; i < NUM_FILAS; i++) {
            proxima_requisicao[i] = exponencial(media_inter_requisicoes[i]);
        }
        
        FILE *arquivo_saida = fopen(nome_arquivo, "w");
        if (arquivo_saida == NULL) {
            printf("Erro ao abrir o arquivo de saida: %s\n", nome_arquivo);
            return 1; 
        }
        fprintf(arquivo_saida, "Tempo(s),Fila1,Fila2,Fila3,TotalSistema,ServidorOcupado,E[N],E[W]\n");


        /*
         * ===================================================================
         * Bloco 3: MOTOR DE EVENTOS (idêntico ao código anterior)
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
             * Bloco 4: PROCESSAMENTO DE EVENTOS (idêntico ao código anterior)
             * ===================================================================
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

                    T_j[fila_idx]++;
                    S_j[fila_idx] += tempo_decorrido;

                } else {
                    perdas[fila_idx]++;
                }
                
                proxima_requisicao[fila_idx] = tempo_decorrido + exponencial(media_inter_requisicoes[fila_idx]);

            } else if (tipo_evento == 3) {
                total_servicos_completos++;
                E_N.qt_requisicoes--;
                E_W_saidas.qt_requisicoes++;

                int fila_que_saiu = fila_em_atendimento;
                double atraso = tempo_decorrido - tempo_chegada_em_atendimento;
                D_j_dep[fila_que_saiu] += atraso;
                
                servidor_ocupado = false;
                fila_em_atendimento = -1;

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
             * Bloco 5: ESCALONADOR PAD (idêntico ao código anterior)
             * ===================================================================
             */
            if (!servidor_ocupado) {
                int fila_a_servir = -1;
                double max_prioridade = -1.0; 

                for (int i = 0; i < NUM_FILAS; i++) {
                    if (tamanho_fila[i] > 0) {
                        
                        double n = (double)tamanho_fila[i];
                        double t = tempo_decorrido;
                        double S = S_j[i];
                        double D_dep = D_j_dep[i];
                        double T = (double)T_j[i];
                        double delta_val = deltas[i];
                        
                        double atraso_medio = 0.0;
                        if (T > 0) {
                            atraso_medio = (n * t - S + D_dep) / T;
                        }

                        double prioridade = delta_val * atraso_medio;

                        if (prioridade > max_prioridade) {
                            max_prioridade = prioridade;
                            fila_a_servir = i;
                        }
                    }
                }

                if (fila_a_servir != -1) {
                    No* no_atendido = cabeca_fila[fila_a_servir];
                    
                    tempo_chegada_em_atendimento = no_atendido->req.tempo_chegada;
                    fila_em_atendimento = fila_a_servir;

                    S_j[fila_a_servir] -= tempo_chegada_em_atendimento;

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
            }
        } // Fim do while(tempo_decorrido < tempo_simulacao)

        /*
         * ===================================================================
         * Bloco 6: RESULTADOS E LIMPEZA DA RODADA
         * ===================================================================
         */
        tempo_decorrido = tempo_simulacao;
        double delta_t = tempo_decorrido - E_N.tempo_anterior;
        E_N.soma_area += delta_t * E_N.qt_requisicoes;
        E_W_chegadas.soma_area += delta_t * E_W_chegadas.qt_requisicoes;
        E_W_saidas.soma_area += delta_t * E_W_saidas.qt_requisicoes;

        printf("---=== Simulacao Finalizada (Ocupacao %s%%) ===---\n", rho_label);
        printf("   Tempo total de simulacao: %.2f segundos\n", tempo_decorrido);
        printf("   Total de chegadas ao sistema: %lu\n", total_chegadas);
        printf("   Total de servicos completos: %lu\n", total_servicos_completos);
        for (int i = 0; i < NUM_FILAS; i++) {
            printf("   Clientes perdidos na Fila %d: %lu\n", i + 1, perdas[i]);
        }

        printf("\n   --- Metricas de Desempenho (%s%%) ---\n", rho_label);
        double ocupacao_calculada = soma_tempo_servico / tempo_decorrido;
        printf("   Ocupacao calculada do servidor: %f (Alvo: %.3f)\n", ocupacao_calculada, ocupacao_atual);
            
        printf("\n   --- Lei de Little (%s%%) ---\n", rho_label);
        double E_N_final = E_N.soma_area / tempo_decorrido;
        double lambda_efetivo = (double)E_W_chegadas.qt_requisicoes / tempo_decorrido;
        double E_W_final = 0.0;
        if (E_W_chegadas.qt_requisicoes > 0){
            E_W_final = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
        }
        double erro_little = E_N_final - lambda_efetivo * E_W_final;  

        printf("   E[N] (numero medio de clientes no sistema): %f\n", E_N_final);
        printf("   E[W] (tempo medio do cliente no sistema): %f\n", E_W_final);
        printf("   Lambda Efetivo (taxa de chegada real): %f\n", lambda_efetivo);
        printf("   Erro numerico (Little): %e\n", erro_little);

        fclose(arquivo_saida);
        
        // Limpa a memória das filas para a próxima rodada
        for(int i = 0; i < NUM_FILAS; i++){
            No* atual = cabeca_fila[i];
            while(atual != NULL){
                No* temp = atual;
                atual = atual->proximo;
                free(temp);
            }
        }

    } // Fim do for (int run = 0; run < NUM_RUNS; run++)

    printf("\n---=== Bateria de Simulacoes Concluida ===---\n");
    return 0;
}