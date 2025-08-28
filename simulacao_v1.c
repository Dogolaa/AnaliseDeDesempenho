#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>

double aleatorio() {
	double u = rand() / ((double) RAND_MAX + 1);
	//limitando entre (0,1]
	u = 1.0 - u;

	return (u);
}

double exponencial(double l){
    return (-1.0/l)*log(aleatorio());
}

double minimal(double n1, double n2){
    return (n1 < n2) ? n1 : n2;
}

void main(){
    srand(time(NULL));

    double tempo_decorrido = 0.0;
    double tempo_simulacao = 86400.0;

    double media_inter_requisicoes;
    double media_tempo_servicos;
    double proxima_requisicao;
    double tempo_servico;

    unsigned long int fila = 0;

    unsigned long int max_fila = 0;

    unsigned long int qtd_requisicoes = 0;

    double soma_inter_requisicoes = 0.0;

    unsigned long int qtd_servicos = 0;
    double soma_tempo_servico = 0.0;

    printf("Informe a media de tempo entre requisições: ");
    scanf("%lF", &media_inter_requisicoes);
    media_inter_requisicoes = 1.0/media_inter_requisicoes;

    printf("Informe a media de tempo para atendimento: ");
    scanf("%lF", &media_tempo_servicos);
    media_tempo_servicos = 1.0/media_tempo_servicos;

    proxima_requisicao = exponencial(media_inter_requisicoes);

    qtd_requisicoes++;
    soma_inter_requisicoes = proxima_requisicao;

    while (tempo_decorrido < tempo_simulacao)
    {
        tempo_decorrido = fila ? minimal(proxima_requisicao, tempo_servico) : proxima_requisicao;
        printf("tempo decorrido: %lF\n", tempo_decorrido);
        if(tempo_decorrido == proxima_requisicao){
            printf("chegada: %lF\n", tempo_decorrido);
            fila ++;

            max_fila = fila > max_fila? fila : max_fila;

            if(fila == 1){
                tempo_servico = tempo_decorrido + exponencial(media_tempo_servicos);

                qtd_servicos++;
                soma_tempo_servico += tempo_servico - tempo_decorrido;
            }

            proxima_requisicao = tempo_decorrido + exponencial(media_inter_requisicoes);

            qtd_requisicoes++;
            soma_inter_requisicoes += proxima_requisicao - tempo_decorrido;
        }else{
            printf("saida: %lF\n", tempo_decorrido);
            fila--;

            if(fila){
                tempo_servico = tempo_decorrido + exponencial(media_tempo_servicos);
                qtd_servicos++;
                soma_tempo_servico += tempo_servico - tempo_decorrido;
            }
        }
        //printf("fila: %ld\n", fila);
        // getchar();
        // getchar();
    }
    printf("---------------------------\n");
    printf("Metricas de validação \n");
    printf("max fila: %ld\n", max_fila);
    printf("media requisicoes: %lF\n", soma_inter_requisicoes / qtd_requisicoes);
    printf("media servico: %lF\n", soma_tempo_servico / qtd_servicos);
    
}