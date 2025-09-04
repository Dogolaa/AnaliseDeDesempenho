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

void main(){
    srand(time(NULL));

    /**
     * declaracao little
     */
    medida_little E_N;
    medida_little E_W_chegadas;
    medida_little E_W_saidas;
    /**
     * iniciando variaveis de little
     */
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
    double tempo_servico = tempo_simulacao * 2; // Inicia com valor alto para não interferir

    //fila
    unsigned long int fila = 0;
    unsigned long int max_fila = 0;

    //variaveis para validacao matematica
    unsigned long int qtd_requisicoes = 0;
    double soma_inter_requisicoes = 0.0;
    unsigned long int qtd_servicos = 0;
    double soma_tempo_servico = 0.0;
    

    printf("Informe a media de tempo entre requisicoes: ");
    // CORREÇÃO: %lf (L minúsculo) é o correto para double
    scanf("%lf", &media_inter_requisicoes);
    //precisamos do valor do parametro l para
    //gerar os numeros pseudo-aleatorios.
    //lembre-se que l = 1.0/media.
    media_inter_requisicoes = 1.0/media_inter_requisicoes;

    printf("Informe a media de tempo para atendimentos: ");
    // CORREÇÃO: %lf (L minúsculo) é o correto para double
    scanf("%lf", &media_tempo_servico);
    //de maneira semelhante...
    media_tempo_servico = 1.0/media_tempo_servico;

    //gerando o tempo de chegada da primeira requisição
    proxima_requisicao = exponencial(media_inter_requisicoes);

    qtd_requisicoes++;
    soma_inter_requisicoes = proxima_requisicao;
    
    // ==================================================================
    // INÍCIO DA LÓGICA CORRIGIDA
    // ==================================================================
    while(tempo_decorrido < tempo_simulacao){
        
        // Se a fila estiver vazia, o próximo evento é certamente uma chegada.
        // Se não, é o mínimo entre a próxima chegada e o término do serviço atual.
        double proximo_evento = fila ? 
            minimo(proxima_requisicao, tempo_servico) : 
            proxima_requisicao;

        // Avança o tempo da simulação para o próximo evento
        tempo_decorrido = proximo_evento;

        // Se o próximo evento ultrapassa o tempo de simulação, paramos.
        if (tempo_decorrido >= tempo_simulacao) {
            break;
        }

        printf("tempo_decorrido: %lf\n", tempo_decorrido);

        // --- TRATAMENTO DE EVENTOS ---
        // É uma boa prática tratar saídas primeiro para liberar recursos do sistema.

        // Evento de SAÍDA: Ocorre se a fila não estava vazia E o tempo de serviço terminou.
        if (fila > 0 && tempo_decorrido == tempo_servico) {
            //printf("saída: %lf\n", tempo_decorrido);
            fila--;

            // Se ainda há gente na fila, inicia o próximo atendimento
            if (fila > 0) {
                double duracao_servico = exponencial(media_tempo_servico);
                tempo_servico = tempo_decorrido + duracao_servico;
                
                qtd_servicos++;
                soma_tempo_servico += duracao_servico;
            }

            /**
             * little para saídas
             */
            E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
            E_N.qt_requisicoes--;
            E_N.tempo_anterior = tempo_decorrido;

            E_W_saidas.soma_area += (tempo_decorrido - E_W_saidas.tempo_anterior) * E_W_saidas.qt_requisicoes;
            E_W_saidas.qt_requisicoes++;
            E_W_saidas.tempo_anterior = tempo_decorrido;
        }

        // Evento de CHEGADA: Ocorre se uma nova requisição chegou.
        if (tempo_decorrido == proxima_requisicao) {
            //printf("chegada: %lf\n", tempo_decorrido);
            fila++;
            max_fila = fila > max_fila ? fila : max_fila;

            // Se a fila estava vazia (agora tem 1 pessoa), o sistema estava ocioso.
            // Inicia o atendimento imediatamente.
            if (fila == 1) {
                double duracao_servico = exponencial(media_tempo_servico);
                tempo_servico = tempo_decorrido + duracao_servico;

                qtd_servicos++;
                soma_tempo_servico += duracao_servico;
            }

            // Agenda a próxima chegada
            double inter_chegada = exponencial(media_inter_requisicoes);
            proxima_requisicao = tempo_decorrido + inter_chegada;

            qtd_requisicoes++;
            soma_inter_requisicoes += inter_chegada;
            
            /**
             * little para chegadas
             */
            E_N.soma_area += (tempo_decorrido - E_N.tempo_anterior) * E_N.qt_requisicoes;
            E_N.qt_requisicoes++;
            E_N.tempo_anterior = tempo_decorrido;

            E_W_chegadas.soma_area += (tempo_decorrido - E_W_chegadas.tempo_anterior) * E_W_chegadas.qt_requisicoes;
            E_W_chegadas.qt_requisicoes++;
            E_W_chegadas.tempo_anterior = tempo_decorrido;
        }
    }
    // ==================================================================
    // FIM DA LÓGICA CORRIGIDA
    // ==================================================================

    // Ajuste final para o cálculo da área na Lei de Little
    // Considera o tempo desde o último evento até o final da simulação
    E_N.soma_area += (tempo_simulacao - E_N.tempo_anterior) * E_N.qt_requisicoes;
    tempo_decorrido = tempo_simulacao;


    printf("\n---=== Métricas e Validações ===---\n");
    printf("max fila: %lu\n", max_fila);
    printf("media entre requisicoes: %lf\n", 
        soma_inter_requisicoes/qtd_requisicoes);
    printf("media tempos de sevico: %lf\n", 
        soma_tempo_servico/qtd_servicos);
    printf("ocupacao esperada:%lf\n", (1.0/media_inter_requisicoes)/(1.0/media_tempo_servico));
    printf("ocupacao calculada:%lf\n", soma_tempo_servico/tempo_decorrido);
    printf("\n--== Lei de Little ==--\n");
    double E_N_final = E_N.soma_area/tempo_decorrido;
    double E_W_final = (E_W_chegadas.soma_area - E_W_saidas.soma_area)/E_W_chegadas.qt_requisicoes;
    double lambda = E_W_chegadas.qt_requisicoes/tempo_decorrido;
    double erro_little = E_N_final - lambda * E_W_final;
    
    printf("E[N]: %lf\n", 
        E_N_final);
    printf("E[W]: %lf\n", 
        E_W_final);
    printf("Erro little: %lf\n", 
        erro_little);
}  
