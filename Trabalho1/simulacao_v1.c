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
    medidas-> qt_requisicoes = 0;
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


    FILE *arquivo_saida; 
    arquivo_saida = fopen("relatorio_simulacao.csv", "w");


    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saída!\n");
        return 1; 
    }

    fprintf(arquivo_saida, "Tempo(s),E[N],E[W],ErroLittle\n");

    int proximo_ponto_relatorio = 10;
    
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

            // little

            E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
            E_N.qt_requisicoes++;
            E_N.tempo_anterior = tempo_decorrido;

            E_W_chegadas.soma_area += (tempo_decorrido - E_W_chegadas.tempo_anterior) 
                * E_W_chegadas.qt_requisicoes;
            E_W_chegadas.qt_requisicoes++;
            E_W_chegadas.tempo_anterior = tempo_decorrido;

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

                        // little

            E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
            E_N.qt_requisicoes--;
            E_N.tempo_anterior = tempo_decorrido;

            E_W_saidas.soma_area += (tempo_decorrido - E_W_saidas.tempo_anterior) * E_W_saidas.qt_requisicoes;
            E_W_saidas.qt_requisicoes++;
            E_W_saidas.tempo_anterior = tempo_decorrido;
        }
        //printf("fila: %d\n============\n", fila);
        //getchar();
        //getchar();

        if (tempo_decorrido >= proximo_ponto_relatorio) {
            // Calcula as métricas com os valores ATUAIS
            double E_N_atual = E_N.soma_area / tempo_decorrido;
            double E_W_atual = 0.0;
            double erro_little_atual = 0.0;
            
            if (E_W_chegadas.qt_requisicoes > 0) {
                E_W_atual = (E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
                double lambda_atual = E_W_chegadas.qt_requisicoes / tempo_decorrido;
                erro_little_atual = E_N_atual - lambda_atual * E_W_atual;
            }

            // Escreve os dados no arquivo em formato CSV
            fprintf(arquivo_saida, "%d,%lf,%lf,%lf\n",
                    proximo_ponto_relatorio,
                    E_N_atual,
                    E_W_atual,
                    erro_little_atual);

            // Avança para o próximo ponto de relatório
            proximo_ponto_relatorio += 10;
        }
    }

    E_W_chegadas.soma_area += (tempo_decorrido - E_W_chegadas.tempo_anterior) 
                * E_W_chegadas.qt_requisicoes;


    E_W_saidas.soma_area += (tempo_decorrido - E_W_saidas.tempo_anterior) 
                * E_W_saidas.qt_requisicoes;

    printf("\n---=== Métricas e Validações ===---\n");
    printf("max fila: %ld\n", max_fila);
    printf("media entre requisicoes: %lF\n", 
        soma_inter_requisicoes/qtd_requisicoes);
    printf("media tempos de sevico: %lF\n", 
        soma_tempo_servico/qtd_servicos);
    printf("Ocupação esperada: %lF\n", 
        media_inter_requisicoes/media_tempo_servico);
    printf("Ocupação calculada: %lF\n", 
        soma_tempo_servico/tempo_decorrido);
        
    printf("\n---=== Lei de Little ===---\n");

    double E_N_final = E_N.soma_area/tempo_decorrido;
    double E_W_final = ( E_W_chegadas.soma_area - E_W_saidas.soma_area) / E_W_chegadas.qt_requisicoes;
    double lambda = E_W_chegadas.qt_requisicoes/tempo_decorrido;
    double erro_little = E_N_final - lambda * E_W_final;  

    printf("E[N]: %lF\n", E_N_final );
    printf("E[W]: %lF\n", E_W_final );
    printf("Erro Little: %lF\n", erro_little );

    return 0;
}