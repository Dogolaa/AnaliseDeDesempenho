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

void main(){
    srand(time(NULL));

    double l;
    printf("Informe a media: ");
    scanf("%lF", &l);
    printf("media informada: %lF\n", l);
    l = 1.0/l;
    printf("valor do parametro l: %lF\n", l);

    double soma = 0;
    for(int i = 0;i <10000; i++){
        double valor = (-1.0/l)*log(aleatorio());
        printf("valor gerado: %lF\n", valor);
        getchar();
        soma += valor;
    }

    printf("media: %lF\n", soma/10000.0);

}#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>

double aleatorio() {
	double u = rand() / ((double) RAND_MAX + 1);
	//limitando entre (0,1]
	u = 1.0 - u;

	return (u);
}

void main(){
    srand(time(NULL));

    double l;
    printf("Informe a media: ");
    scanf("%lF", &l);
    printf("media informada: %lF\n", l);
    l = 1.0/l;
    printf("valor do parametro l: %lF\n", l);

    double soma = 0;
    for(int i = 0;i <10000; i++){
        double valor = (-1.0/l)*log(aleatorio());
        printf("valor gerado: %lF\n", valor);
        getchar();
        soma += valor;
    }

    printf("media: %lF\n", soma/10000.0);

}