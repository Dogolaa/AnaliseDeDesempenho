#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>

typedef struct {
    double tempo_anterior;
    unsigned long int qt_requisicoes;
    double soma_area;
} medida_little;

double aleatorio() {
    double u = rand() / ((double) RAND_MAX + 1);
    //limitando entre (0,1]
    u = 1.0 - u;
    return (u);
}

double exponencial(double l){
    return (-1.0/l)*log(aleatorio());
}

double minimo(double n1, double n2){
    if(n1 < n2) return n1;
    return n2;
}

void inicia_little(medida_little * medidas){
    medidas->tempo_anterior = 0.0;
    medidas->qt_requisicoes = 0;
    medidas->soma_area = 0.0;
}

// CORREÇÃO: Usar "int main(void)" é o padrão em C, em vez de "void main()"
int main(void){
    srand(time(NULL));

    medida_little E_N;
    medida_little E_W_chegadas;
    medida_little E_W_saidas;
    inicia_little(&E_N);
    inicia_little(&E_W_chegadas);
    inicia_little(&E_W_saidas);
    
    double tempo_decorrido = 0.0;
    double tempo_simulacao = 86400.0;
    double media_inter_requisicoes;
    double media_tempo_servico;
    double proxima_requisicao;
    // INICIALIZAÇÃO: Inicia tempo_servico com valor alto para não ser escolhido por engano
    double tempo_servico = tempo_simulacao * 2; 

    unsigned long int fila = 0;
    unsigned long int max_fila = 0;

    unsigned long int qtd_requisicoes = 0;
    // INICIALIZAÇÃO: Boas práticas de inicializar a variável
    double soma_inter_requisicoes = 0.0; 
    unsigned long int qtd_servicos = 0;
    double soma_tempo_servico = 0.0;
    
    // CORREÇÃO: Usar "%lf" para ler um double com scanf
    printf("Informe a media de tempo entre requisicoes: ");
    scanf("%lf", &media_inter_requisicoes);
    media_inter_requisicoes = 1.0/media_inter_requisicoes;

    printf("Informe a media de tempo para atendimentos: ");
    scanf("%lf", &media_tempo_servico);
    media_tempo_servico = 1.0/media_tempo_servico;

    proxima_requisicao = exponencial(media_inter_requisicoes);
    qtd_requisicoes++;
    soma_inter_requisicoes = proxima_requisicao;
    
    FILE *arquivo_saida = fopen("relatorio_simulacao999.csv", "w");
    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saída!\n");
        return 1; 
    }
    fprintf(arquivo_saida, "Tempo(s),Fila,E[N],E[W]\n");
    double proximo_ponto_relatorio = 10.0;

    while(tempo_decorrido < tempo_simulacao){
        double tempo_proximo_evento = minimo(proxima_requisicao, proximo_ponto_relatorio);
        if (fila > 0) {
            tempo_proximo_evento = minimo(tempo_proximo_evento, tempo_servico);
        }

        // Avança o relógio para o próximo evento
        tempo_decorrido = tempo_proximo_evento;

        if (tempo_decorrido > tempo_simulacao) {
            break;
        }

        // Identifica e processa o evento
        if(tempo_decorrido == proxima_requisicao){ // --- EVENTO DE CHEGADA ---
            // Atualiza a área ANTES de mudar o estado (lógica original preservada)
            E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
            E_W_chegadas.soma_area += (tempo_decorrido - E_W_chegadas.tempo_anterior) * E_W_chegadas.qt_requisicoes;
            
            // Atualiza o estado do sistema
            fila++;
            max_fila = fila > max_fila ? fila : max_fila;
            E_N.qt_requisicoes++;
            E_W_chegadas.qt_requisicoes++;
            E_N.tempo_anterior = tempo_decorrido;
            E_W_chegadas.tempo_anterior = tempo_decorrido;

            if(fila == 1){ // Inicia serviço se estava ocioso
                tempo_servico = tempo_decorrido + exponencial(media_tempo_servico);
                qtd_servicos++;
                soma_tempo_servico += tempo_servico - tempo_decorrido;
            }

            // Agenda próxima chegada
            proxima_requisicao = tempo_decorrido + exponencial(media_inter_requisicoes);
            qtd_requisicoes++;
            soma_inter_requisicoes += proxima_requisicao - tempo_decorrido;

        } else if (fila > 0 && tempo_decorrido == tempo_servico) { // --- EVENTO DE SAÍDA ---
            // Atualiza a área ANTES de mudar o estado
            E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
            E_W_saidas.soma_area += (tempo_decorrido - E_W_saidas.tempo_anterior) * E_W_saidas.qt_requisicoes;
            
            // Atualiza o estado do sistema
            fila--;
            E_N.qt_requisicoes--;
            E_W_saidas.qt_requisicoes++;
            E_N.tempo_anterior = tempo_decorrido;
            E_W_saidas.tempo_anterior = tempo_decorrido;

            if(fila > 0){ // Inicia próximo serviço se houver fila
                tempo_servico = tempo_decorrido + exponencial(media_tempo_servico);
                qtd_servicos++;
                soma_tempo_servico += tempo_servico - tempo_decorrido;
            } else { // Fila ficou vazia
                tempo_servico = tempo_simulacao * 2;
            }

        } else { // --- EVENTO DE RELATÓRIO ---
            // Atualiza a área de todas as métricas até o momento do relatório
            E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
            E_W_chegadas.soma_area += (tempo_decorrido - E_W_chegadas.tempo_anterior) * E_W_chegadas.qt_requisicoes;
            E_W_saidas.soma_area += (tempo_decorrido - E_W_saidas.tempo_anterior) * E_W_saidas.qt_requisicoes;
            
            // Atualiza o tempo anterior para o momento atual
            E_N.tempo_anterior = tempo_decorrido;
            E_W_chegadas.tempo_anterior = tempo_decorrido;
            E_W_saidas.tempo_anterior = tempo_decorrido;

            // Calcula as métricas parciais
            double E_N_atual = E_N.soma_area / tempo_decorrido;
            double E_W_atual = 0.0;
            if (E_W_chegadas.qt_requisicoes > 0) {
                E_W_atual = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
            }

            // Imprime no terminal para feedback
            printf("Tempo: %.0fs | Fila: %lu | E[N] parcial: %f\n", tempo_decorrido, fila, E_N_atual);

            // Salva no arquivo
            fprintf(arquivo_saida, "%.0f,%lu,%f,%f\n", tempo_decorrido, fila, E_N_atual, E_W_atual);
            fflush(arquivo_saida); // Força a escrita no disco

            // Agenda o próximo relatório
            proximo_ponto_relatorio += 10.0;
        }
    }

    tempo_decorrido = tempo_simulacao;
    E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
    E_W_chegadas.soma_area += (tempo_decorrido - E_W_chegadas.tempo_anterior) * E_W_chegadas.qt_requisicoes;
    E_W_saidas.soma_area += (tempo_decorrido - E_W_saidas.tempo_anterior) * E_W_saidas.qt_requisicoes;

    printf("\n---=== Métricas e Validações Finais ===---\n");
    // CORREÇÃO: Usar "%f" para imprimir um double
    printf("max fila: %ld\n", max_fila);
    printf("media entre requisicoes: %f\n", soma_inter_requisicoes/qtd_requisicoes);
    printf("media tempos de sevico: %f\n", soma_tempo_servico/qtd_servicos);
    printf("Ocupação esperada: %f\n", (1.0/media_inter_requisicoes)/(1.0/media_tempo_servico));
    printf("Ocupação calculada: %f\n", soma_tempo_servico/tempo_decorrido);
        
    printf("\n---=== Lei de Little ===---\n");

    double E_N_final = E_N.soma_area / tempo_decorrido;
    double E_W_final = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
    double lambda = (double)E_W_chegadas.qt_requisicoes / tempo_decorrido;
    double erro_little = E_N_final - lambda * E_W_final;  

    printf("E[N]: %f\n", E_N_final);
    printf("E[W]: %f\n", E_W_final);
    // Usar %e para ver números muito pequenos (erros)
    printf("Erro Little: %e\n", erro_little);

    fclose(arquivo_saida); // Fecha o arquivo
    return 0; // Boa prática para int main
}