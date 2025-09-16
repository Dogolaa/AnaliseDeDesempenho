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
    u = 1.0 - u;
    return u;
}

double exponencial(double l){
    return (-1.0/l) * log(aleatorio());
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
    double tempo_servico = tempo_simulacao * 2;

    unsigned long int fila = 0;
    unsigned long int max_fila = 0;

    unsigned long int qtd_requisicoes = 0;
    double soma_inter_requisicoes = 0.0;
    unsigned long int qtd_servicos = 0;
    double soma_tempo_servico = 0.0;
    
    printf("Informe a media de tempo entre requisicoes: ");
    scanf("%lf", &media_inter_requisicoes);
    media_inter_requisicoes = 1.0/media_inter_requisicoes;

    printf("Informe a media de tempo para atendimentos: ");
    scanf("%lf", &media_tempo_servico);
    media_tempo_servico = 1.0/media_tempo_servico;

    proxima_requisicao = exponencial(media_inter_requisicoes);
    
    FILE *arquivo_saida = fopen("relatorio_simulacao_ocupacao80.csv", "w");
    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saída!\n");
        return 1; 
    }
    fprintf(arquivo_saida, "Tempo(s),E[N],E[W],ErroLittle\n");

    
    while(tempo_decorrido < tempo_simulacao){
        
        double tempo_proximo_evento = minimo(proxima_requisicao, proximo_ponto_relatorio);
        if (fila > 0) {
            tempo_proximo_evento = minimo(tempo_proximo_evento, tempo_servico);
        }

        E_N.soma_area += (tempo_proximo_evento - tempo_decorrido) * E_N.qt_requisicoes;
        E_W_chegadas.soma_area += (tempo_proximo_evento - tempo_decorrido) * E_W_chegadas.qt_requisicoes;
        E_W_saidas.soma_area += (tempo_proximo_evento - tempo_decorrido) * E_W_saidas.qt_requisicoes;

        tempo_decorrido = tempo_proximo_evento;

        if (tempo_decorrido > tempo_simulacao) break;

        if(tempo_decorrido == proxima_requisicao){ 
            fila++;
            E_N.qt_requisicoes++;
            E_W_chegadas.qt_requisicoes++;
            max_fila = fila > max_fila ? fila : max_fila;

            if(fila == 1){
                tempo_servico = tempo_decorrido + exponencial(media_tempo_servico);
                soma_tempo_servico += (tempo_servico - tempo_decorrido);
                qtd_servicos++;
            }

            proxima_requisicao = tempo_decorrido + exponencial(media_inter_requisicoes);
        
        } else if (fila > 0 && tempo_decorrido == tempo_servico) { 
            fila--;
            E_N.qt_requisicoes--;
            E_W_saidas.qt_requisicoes++;

            if(fila > 0){ 
                tempo_servico = tempo_decorrido + exponencial(media_tempo_servico);
                soma_tempo_servico += (tempo_servico - tempo_decorrido);
                qtd_servicos++;
            } else {
                tempo_servico = tempo_simulacao * 2;
            }

        } else { 
            double E_N_atual = E_N.soma_area / tempo_decorrido;
            double E_W_atual = 0.0;
            double erro_little_atual = 0.0;
            
            if (E_W_chegadas.qt_requisicoes > 0) {
                E_W_atual = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
                double lambda_atual = (double)E_W_chegadas.qt_requisicoes / tempo_decorrido;
                erro_little_atual = E_N_atual - lambda_atual * E_W_atual;
            }

            fprintf(arquivo_saida, "%.0f,%lf,%lf,%lf\n",
                    tempo_decorrido, E_N_atual, E_W_atual, erro_little_atual);

            proximo_ponto_relatorio += 10.0;
        }
    }

    tempo_decorrido = tempo_simulacao;
    printf("\n---=== Métricas e Validações Finais ===---\n");
    printf("Tempo total simulado: %.2f\n", tempo_decorrido);
    printf("Max fila: %ld\n", max_fila);
        
    printf("\n---=== Lei de Little (Final) ===---\n");
    double E_N_final = E_N.soma_area / tempo_decorrido;
    double E_W_final = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
    double lambda = (double)E_W_chegadas.qt_requisicoes / tempo_decorrido;
    double erro_little = E_N_final - lambda * E_W_final;  

    printf("E[N]: %lf\n", E_N_final);
    printf("Lambda: %lf\n", lambda);
    printf("E[W]: %lf\n", E_W_final);
    printf("Erro Little: %e\n", erro_little);

    fclose(arquivo_saida);
    return 0;
}