#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<stdbool.h> // Para usar o tipo bool (true/false)

#define NUM_FILAS 3 // Define o número de filas a serem gerenciadas

// --- ESTRUTURAS DE DADOS NOVAS ---
// Estrutura para representar uma única requisição (cliente)
typedef struct {
    double tempo_chegada;
} Requisicao;

// Estrutura para o nó da lista ligada (nossa fila)
typedef struct No {
    Requisicao req;
    struct No* proximo;
} No;

// Estrutura para as medidas de Little (sem alterações)
typedef struct {
    double tempo_anterior;
    unsigned long int qt_requisicoes; // No caso de E_N, será o total no sistema
    double soma_area;
} medida_little;

// --- FUNÇÕES AUXILIARES ---
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

// --- FUNÇÃO PRINCIPAL ---
int main(void){
    srand(time(NULL));

    // --- VARIÁVEIS DA SIMULAÇÃO ---
    medida_little E_N;
    medida_little E_W_chegadas;
    medida_little E_W_saidas;
    inicia_little(&E_N);
    inicia_little(&E_W_chegadas);
    inicia_little(&E_W_saidas);
    
    double tempo_decorrido = 0.0;
    double tempo_simulacao = 86400.0; // 1 dia em segundos
    bool servidor_ocupado = false;

    // Variáveis para as 3 filas
    double media_inter_requisicoes[NUM_FILAS];
    double proxima_requisicao[NUM_FILAS];
    
    // As filas agora são listas ligadas
    No* cabeca_fila[NUM_FILAS] = {NULL};
    No* cauda_fila[NUM_FILAS] = {NULL};
    unsigned long int tamanho_fila[NUM_FILAS] = {0};
    unsigned long int max_fila; // Tamanho máximo para cada fila
    unsigned long int perdas[NUM_FILAS] = {0}; // Contagem de clientes perdidos por fila cheia

    // Variáveis de serviço (um único servidor)
    double media_tempo_servico;
    double tempo_saida_servico = tempo_simulacao * 2; // Inicializa com valor "infinito"

    // Variáveis de estatística
    unsigned long int total_chegadas = 0;
    unsigned long int total_servicos_completos = 0;
    double soma_tempo_servico = 0.0;
    
    // --- COLETA DE PARÂMETROS ---
    printf("---=== Simulador de Fila Única com Múltiplas Filas ===---\n");
    for (int i = 0; i < NUM_FILAS; i++) {
        printf("Informe a taxa de chegada da Fila %d (reqs/segundo): ", i + 1);
        scanf("%lf", &media_inter_requisicoes[i]);
    }
    printf("Informe a taxa de atendimento do servidor (reqs/segundo): ");
    scanf("%lf", &media_tempo_servico);
    printf("Informe o tamanho maximo para cada fila: ");
    scanf("%lu", &max_fila);

    // Inicializa os agendadores de chegada para cada fila
    for (int i = 0; i < NUM_FILAS; i++) {
        proxima_requisicao[i] = exponencial(media_inter_requisicoes[i]);
    }
    
    // --- ARQUIVO DE SAÍDA ---
    FILE *arquivo_saida = fopen("relatorio_simulacao.csv", "w");
    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saida!\n");
        return 1; 
    }
    fprintf(arquivo_saida, "Tempo(s),Fila1,Fila2,Fila3,TotalSistema,ServidorOcupado,E[N],E[W]\n");
    double proximo_ponto_relatorio = 10.0;

    // --- LAÇO PRINCIPAL DA SIMULAÇÃO ---
    while(tempo_decorrido < tempo_simulacao){
        
        // --- DETERMINAÇÃO DO PRÓXIMO EVENTO ---
        double tempo_proximo_evento = tempo_simulacao * 2;
        int tipo_evento = -1; // -1: nenhum, 0-2: chegada na fila 0-2, 3: saida, 4: relatorio

        // Verifica a próxima chegada em cada fila
        for (int i = 0; i < NUM_FILAS; i++) {
            if (proxima_requisicao[i] < tempo_proximo_evento) {
                tempo_proximo_evento = proxima_requisicao[i];
                tipo_evento = i; // Evento de chegada na fila 'i'
            }
        }
        // Verifica a próxima saída (se o servidor estiver ocupado)
        if (servidor_ocupado && tempo_saida_servico < tempo_proximo_evento) {
            tempo_proximo_evento = tempo_saida_servico;
            tipo_evento = 3; // Evento de saída
        }
        // Verifica o próximo ponto de relatório
        if (proximo_ponto_relatorio < tempo_proximo_evento) {
            tempo_proximo_evento = proximo_ponto_relatorio;
            tipo_evento = 4; // Evento de relatório
        }

        // Avança o relógio para o próximo evento
        tempo_decorrido = tempo_proximo_evento;

        if (tempo_decorrido > tempo_simulacao) {
            break;
        }

        // --- ATUALIZAÇÃO DAS MÉTRICAS DE LITTLE ANTES DE ALTERAR O ESTADO ---
        double delta_t = tempo_decorrido - E_N.tempo_anterior;
        E_N.soma_area += delta_t * E_N.qt_requisicoes;
        E_W_chegadas.soma_area += delta_t * E_W_chegadas.qt_requisicoes;
        E_W_saidas.soma_area += delta_t * E_W_saidas.qt_requisicoes;
        E_N.tempo_anterior = tempo_decorrido;
        E_W_chegadas.tempo_anterior = tempo_decorrido;
        E_W_saidas.tempo_anterior = tempo_decorrido;

        // --- PROCESSAMENTO DO EVENTO ---
        if(tipo_evento >= 0 && tipo_evento < NUM_FILAS) { // --- EVENTO DE CHEGADA NA FILA 'tipo_evento' ---
            int fila_idx = tipo_evento;

            // Verifica se a fila específica tem espaço
            if (tamanho_fila[fila_idx] < max_fila) {
                // Cria a nova requisição
                No* novo_no = (No*) malloc(sizeof(No));
                novo_no->req.tempo_chegada = tempo_decorrido;
                novo_no->proximo = NULL;

                // Adiciona na fila correspondente
                if (cabeca_fila[fila_idx] == NULL) { // Fila estava vazia
                    cabeca_fila[fila_idx] = novo_no;
                    cauda_fila[fila_idx] = novo_no;
                } else { // Adiciona no final
                    cauda_fila[fila_idx]->proximo = novo_no;
                    cauda_fila[fila_idx] = novo_no;
                }
                tamanho_fila[fila_idx]++;
                total_chegadas++;

                // Atualiza contadores de Little
                E_N.qt_requisicoes++;
                E_W_chegadas.qt_requisicoes++;

                // Se o servidor estava livre, já inicia o serviço
                if (!servidor_ocupado) {
                    // Remove o cliente que acabou de chegar (único no sistema)
                    No* no_atendido = cabeca_fila[fila_idx];
                    cabeca_fila[fila_idx] = no_atendido->proximo;
                    if (cabeca_fila[fila_idx] == NULL) cauda_fila[fila_idx] = NULL;
                    tamanho_fila[fila_idx]--;
                    free(no_atendido);
                    
                    // Agenda a saída
                    double duracao_servico = exponencial(media_tempo_servico);
                    tempo_saida_servico = tempo_decorrido + duracao_servico;
                    soma_tempo_servico += duracao_servico;
                    servidor_ocupado = true;
                }
            } else {
                perdas[fila_idx]++; // Fila cheia, cliente perdido
            }
            
            // Agenda a próxima chegada para ESTA fila
            proxima_requisicao[fila_idx] = tempo_decorrido + exponencial(media_inter_requisicoes[fila_idx]);

        } else if (tipo_evento == 3) { // --- EVENTO DE SAÍDA ---
            total_servicos_completos++;
            // Atualiza contadores de Little
            E_N.qt_requisicoes--;
            E_W_saidas.qt_requisicoes++;
            
            // --- LÓGICA DE DECISÃO: SERVIDOR FICOU LIVRE ---
            int fila_a_servir = -1;
            double menor_tempo_chegada = tempo_simulacao * 2;

            // Procura em todas as filas pelo cliente que chegou mais cedo
            for (int i = 0; i < NUM_FILAS; i++) {
                if (cabeca_fila[i] != NULL) { // Se a fila não está vazia
                    if (cabeca_fila[i]->req.tempo_chegada < menor_tempo_chegada) {
                        menor_tempo_chegada = cabeca_fila[i]->req.tempo_chegada;
                        fila_a_servir = i;
                    }
                }
            }

            if (fila_a_servir != -1) { // Encontrou um cliente para servir
                // Remove o cliente da cabeça da fila escolhida
                No* no_atendido = cabeca_fila[fila_a_servir];
                cabeca_fila[fila_a_servir] = no_atendido->proximo;
                if (cabeca_fila[fila_a_servir] == NULL) {
                    cauda_fila[fila_a_servir] = NULL;
                }
                tamanho_fila[fila_a_servir]--;
                free(no_atendido);
                
                // Agenda a próxima saída
                double duracao_servico = exponencial(media_tempo_servico);
                tempo_saida_servico = tempo_decorrido + duracao_servico;
                soma_tempo_servico += duracao_servico;
                servidor_ocupado = true; // Continua ocupado
            } else {
                // Todas as filas estão vazias
                servidor_ocupado = false;
                tempo_saida_servico = tempo_simulacao * 2; // "Infinito"
            }

        } else if (tipo_evento == 4) { // --- EVENTO DE RELATÓRIO ---
            double E_N_atual = E_N.soma_area / tempo_decorrido;
            double E_W_atual = 0.0;
            if (E_W_chegadas.qt_requisicoes > 0) {
                E_W_atual = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
            }

            // Salva no arquivo
            fprintf(arquivo_saida, "%.0f,%lu,%lu,%lu,%lu,%d,%f,%f\n", 
                    tempo_decorrido, 
                    tamanho_fila[0], tamanho_fila[1], tamanho_fila[2], 
                    E_N.qt_requisicoes, 
                    servidor_ocupado, E_N_atual, E_W_atual);
            fflush(arquivo_saida);

            // Agenda o próximo relatório
            proximo_ponto_relatorio += 10.0;
        }
    }

    // --- CÁLCULOS E IMPRESSÃO DOS RESULTADOS FINAIS ---
    tempo_decorrido = tempo_simulacao; // Garante que o tempo final seja o da simulação
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
    // Lambda efetivo (chegadas que entraram no sistema, não as que foram perdidas)
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
    // Liberar memória das filas, se houver algo restante
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