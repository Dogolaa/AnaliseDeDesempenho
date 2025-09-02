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

double minimo(double n1, double n2){
    if(n1 < n2) return n1;
    return n2;
}

void main(){
    srand(time(NULL));
    
    //início 0,0 segundos
    double tempo_decorrido = 0.0;

    //tempo total que desejo simular (24 horas)
    double tempo_simulacao = 86400.0;

    //intervalo entre requisições
    double media_inter_requisicoes;

    //tempo medio gasto em um atendimento
    double media_tempo_servico;

    //marca sempre o tempo de chegada da
    //proxima requisição
    double proxima_requisicao;

    //possui o tempo de serviço da
    //requisição atualmente em atendimento
    //pelo servidor
    double tempo_servico;

    //fila
    unsigned long int fila = 0;
    unsigned long int max_fila = 0;

    //variaveis para validacao matematica
    unsigned long int qtd_requisicoes = 0;
    double soma_inter_requisicoes;
    unsigned long int qtd_servicos = 0;
    double soma_tempo_servico = 0.0;
    

    printf("Informe a media de tempo entre requisicoes: ");
    scanf("%lF", &media_inter_requisicoes);
    //precisamos do valor do parametro l para
    //gerar os numeros pseudo-aleatorios.
    //lembre-se que l = 1.0/media.
    media_inter_requisicoes = 1.0/media_inter_requisicoes;

    printf("Informe a media de tempo para atendimentos: ");
    scanf("%lF", &media_tempo_servico);
    //de maneira semelhante...
    media_tempo_servico = 1.0/media_tempo_servico;

    //gerando o tempo de chegada da primeira requisição
    proxima_requisicao = exponencial(media_inter_requisicoes);

    qtd_requisicoes++;
    soma_inter_requisicoes = proxima_requisicao;
    
    while(tempo_decorrido < tempo_simulacao){
        tempo_decorrido = fila ?
            minimo(proxima_requisicao, tempo_servico) :
            proxima_requisicao;
        printf("tempo_decorrido: %lF\n", tempo_decorrido);

        //hora de tratar os eventos!
        //chegada acontecendo!
        if(tempo_decorrido == proxima_requisicao){
            //printf("chegada: %lF\n", tempo_decorrido);
            fila++;
            max_fila = fila > max_fila ?
                fila :
                max_fila;

            //ambiente estava ocioso!
            //inicia o atendimento imediatamente
            if(fila == 1){
                tempo_servico = tempo_decorrido +
                exponencial(media_tempo_servico);

                qtd_servicos++;
                soma_tempo_servico += 
                    tempo_servico - tempo_decorrido;
            }

            proxima_requisicao = tempo_decorrido + 
            exponencial(media_inter_requisicoes);

            qtd_requisicoes++;
            soma_inter_requisicoes +=
                proxima_requisicao - tempo_decorrido;
        }else{ //saída acontecendo
            //printf("saída: %lF\n", tempo_decorrido);
            fila--;

            if(fila){
                tempo_servico = tempo_decorrido +
                exponencial(media_tempo_servico);
                
                qtd_servicos++;
                soma_tempo_servico += 
                    tempo_servico - tempo_decorrido;
            }
        }
        //printf("fila: %d\n============\n", fila);
        //getchar();
        //getchar();
    }
    printf("\n---=== Métricas e Validações ===---\n");
    printf("max fila: %d\n", max_fila);
    printf("media entre requisicoes: %lF\n", 
        soma_inter_requisicoes/qtd_requisicoes);
    printf("media tempos de sevico: %lF\n", 
        soma_tempo_servico/qtd_servicos);

}