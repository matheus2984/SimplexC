#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <stdbool.h>
#include <math.h>

typedef struct {
	double x[40];
	char tp[3];
	double termo;
} eqSimplex_t;


typedef struct {
	eqSimplex_t item;
	eqSimplex_t* next;
} node_t; // estrutura de nó da lista

typedef struct {
	node_t* inicio;
	node_t* fim;
	int count;
} lista_t; // cabeça da lista

typedef struct {
	lista_t lstEquacoes;
	int qtdVar;
	int qtdRest;
} simplex_t;

void lstInit(lista_t* lst) // inicia uma nova lista
{
	lst->count = 0;
	lst->inicio = NULL;
	lst->fim = NULL;
}

void lstAdd(lista_t* lst, eqSimplex_t item) // adiciona um nó a lista com o valor do item
{
	node_t* node = malloc(sizeof(node_t));
	node->item = item;
	node->next = NULL;
	
	if (lst->count == 0)lst->inicio = node;
	else lst->fim->next = node;

	lst->fim = node;
	lst->count++;
}

void lstFree(lista_t* lst) // limpa memoria alocada para os nós da lista
{
	int i;
	node_t* temp;
	node_t* node = lst->inicio;
	while (node->next)
	{
		temp = node;
		node = node->next;
		free(temp);
	}
	lst->count = 0;
}

void simplexPrint(simplex_t simplex) // imprime a tabela simplex
{
	lista_t lst = simplex.lstEquacoes;

	if (lst.count <= 0) return;
	node_t * node = lst.inicio;
	int i, j;
	for (i = 0; i < lst.count; i++)
	{
		for (j = 0; j < simplex.qtdVar + simplex.qtdRest; j++)
		{
			printf("x%d: %lf\t|", j, node->item.x[j]);
		}
		printf("%lf\n", node->item.termo);
		node = node->next;
	}
}

void printEq(simplex_t s, eqSimplex_t eq) // imprime uma linha especifica da tabela
{
	int i;
	for (i = 0; i < s.qtdVar + s.qtdRest; i++) printf("x%d: %lf\t|", i, eq.x[i]);
	printf(" %s ", eq.tp);
	printf("%lf\n", eq.termo);
}

void adjustsInsertEq(simplex_t* s, eqSimplex_t* eq) // ajusta as equações para <= e as adiciona na lista
{
	int i;
	if (strcmp(eq->tp, "<=") == 0)
	{
		lstAdd(&s->lstEquacoes, *eq);
	}
	else if (strcmp(eq->tp, ">=") == 0)
	{
		for (i = 0; i < s->qtdVar; i++) eq->x[i] *= -1;
		strcpy(eq->tp, "<=");
		eq->termo *= -1;
		lstAdd(&s->lstEquacoes, *eq);
	}
	else
	{
		eqSimplex_t neq1 = *eq;
		strcpy(neq1.tp, "<=");
		lstAdd(&s->lstEquacoes, neq1);
		strcpy(neq1.tp, ">=");
		adjustsInsertEq(s, &neq1);
		s->qtdRest++;
	}
}

void addFolga(simplex_t* s) // adiciona as variaveis de folga em cada equação
{
	node_t * eq = s->lstEquacoes.inicio;
	int i, j;
	for (i = 0; i < s->lstEquacoes.count; i++) {
		for (j = s->qtdVar; j < s->qtdVar + s->qtdRest; j++)
		{
			if (j - s->qtdVar == i) eq->item.x[j] = 1;
			else eq->item.x[j] = 0;
		}

		eq = eq->next;
	}
}

bool needAdjust(simplex_t* s) // verifica se o quadro simplex precisa ser normalizado
{
	int i;
	node_t* eq = s->lstEquacoes.inicio;
	for (i = 0; i < s->lstEquacoes.count; i++) // percorre todas as equações
	{
		if (eq->item.termo < 0) return true; // se algum termo esta negativo retorna verdade (é necessario a normalizar o quadro)
		eq = eq->next;
	}
	return false;
}

void simplexAdjust(simplex_t* s) // normaliza o quadro simplex
{
	int i, j;
	node_t* termoNegativo = s->lstEquacoes.inicio;
	double menorTermo = 0;
	int rowP; // linha do menor termpo (linha que sai pivo);

	for (i = 0; i < s->lstEquacoes.count; i++) // percorre todas as linhas da tabela em busca do menor termo
	{
		if (termoNegativo->item.termo < menorTermo)
		{
			menorTermo = termoNegativo->item.termo;
			rowP = i;
		}
		termoNegativo = termoNegativo->next;
	}

	node_t* linhaPivo = s->lstEquacoes.inicio;
	for (i = 0; i < rowP; i++) linhaPivo = linhaPivo->next; // pega a linha pivo


	node_t* z = s->lstEquacoes.fim; // pega a linha do Z (sempre a ultima linha da lista)
	double menorRazao = DBL_MAX;
	double pivo;
	int colP; // coluna pivo

	for (i = 0; i < s->qtdVar + s->qtdRest; i++)
	{
		if (linhaPivo->item.x[i] < 0) // pega os valores negativos da linha pivo
		{
			double razao = fabs(z->item.x[i] / linhaPivo->item.x[i]);
			if (razao < menorRazao) // pega a menor razao
			{
				pivo = linhaPivo->item.x[i];
				menorRazao = razao;
				colP = i;
			}
		}
	}

	for (i = 0; i < s->qtdVar + s->qtdRest; i++) linhaPivo->item.x[i] /= pivo; // calcula nova linha pivo
	linhaPivo->item.termo /= pivo; // calcula novo termo da linha pivo

	node_t* eq = s->lstEquacoes.inicio; // pega a primeira linha
	for (i = 0; i < s->lstEquacoes.count; i++) // percorre todas as linhas
	{
		if (i != rowP) // se for a linha pivo não faz calculos somente pula para a proxima linha
		{
			double pivoL = eq->item.x[colP]; // pega o elemento da coluna pivo dessa linha
			for (j = 0; j < s->qtdVar + s->qtdRest; j++)
			{
				eq->item.x[j] += linhaPivo->item.x[j] * (pivoL*-1); // soma o elemento da linha com o elemento da nova linha pivo multiplicado pelo pivoL negativo
			}
			eq->item.termo += linhaPivo->item.termo * (pivoL*-1); // calcula novo termo
		}
		eq = eq->next;
	}

	printf("table ajustada\n");
	simplexPrint(*s);
	printf("\n");
	if (needAdjust(s)) simplexAdjust(s); // normaliza novamente se necessario o quadro simplex
}

void simplexSolve(simplex_t* s) // resolve o quadro simplex
{
	if(needAdjust(s)) simplexAdjust(s); // normaliza se necessario o quadro simplex
	
	int i, j;
	double menorZ = 0; //menor valor de Z
	int colZ; // coluna mais negativa de Z (coluna pivo)
	node_t* z = s->lstEquacoes.fim; // objetivo (Z) é sempre o ultimo elemento da lista
	
	for (i = 0; i < s->qtdVar + s->qtdRest; i++) // percorre todas as variaveis de Z
	{
		if (z->item.x[i] < menorZ)
		{
			menorZ = z->item.x[i];
			colZ = i;
		}
	}

	
	if (menorZ >= 0) return; // condição de parada

	node_t* eq = s->lstEquacoes.inicio; // pega a primeira linha
	double melhorPivo = DBL_MAX;
	int rowP; // index da linha pivo
	double pivo; // elemento pivo
	for (i = 0; i < s->lstEquacoes.count - 1; i++) // percorre todas as linhas menos a linha Z
	{
		double pivoCol = eq->item.x[colZ]; // elemento da coluna pivo dessa linha
		double result = eq->item.termo / pivoCol; // divide o termo pelo pivo
		if (fabs(result) < fabs(melhorPivo))
		{
			pivo = pivoCol;
			melhorPivo = result;
			rowP = i;
		}
		eq = eq->next; // pega a proxima linha e repete o ciclo
	}

	//

	node_t * linhaPivo = s->lstEquacoes.inicio;
	for (i = 0; i < rowP; i++) linhaPivo = linhaPivo->next;  // busca a linha pivo

	for (i = 0; i < s->qtdVar + s->qtdRest; i++) // percorre todas as variaveis da linha pivo
	{
		linhaPivo->item.x[i] /= pivo; // divide o elemento da linha pivo pelo pivo (gerando a nova linha pivo)
	}
	linhaPivo->item.termo /= pivo; // divide o termo da linha pivo pelo elemento pivo

	//

	eq = s->lstEquacoes.inicio; // pega a primeira linha
	for (i = 0; i < s->lstEquacoes.count; i++) // percorre todas as linhas
	{
		if (i != rowP) // se for a linha pivo não faz calculos somente pula para a proxima linha
		{
			double pivoL = eq->item.x[colZ]; // pega o elemento da coluna pivo dessa linha
			for (j = 0; j < s->qtdVar + s->qtdRest; j++)
			{
				eq->item.x[j] += linhaPivo->item.x[j] * (pivoL*-1); // soma o elemento da linha com o elemento da nova linha pivo multiplicado pelo pivoL negativo
			}
			eq->item.termo += linhaPivo->item.termo * (pivoL*-1); // calcula novo termo
		}
		eq = eq->next;
	}

	
	simplexSolve(s); // proxima interação
}

void saveFileB(simplex_t s, char * fName) // salva os dados no formato binario
{
	FILE* f = fopen(fName, "wb");
	node_t* node = s.lstEquacoes.inicio;
	fwrite(&s.qtdVar, sizeof(int), 1, f);
	fwrite(&s.qtdRest, sizeof(int), 1, f);
	while (node) 
	{
		fwrite(&node->item, sizeof(eqSimplex_t), 1, f);
		node = node->next;
	}
	fclose(f);
}

void loadFileB(simplex_t* s, char * fName) // carrega os dados no formato binario
{
	FILE* f = fopen(fName, "rb");
	fread(&s->qtdVar, sizeof(int), 1, f);
	fread(&s->qtdRest, sizeof(int), 1, f);

	eqSimplex_t eq;
	while (fread(&eq, sizeof(eqSimplex_t), 1, f) > 0)
	{
		adjustsInsertEq(s, &eq);
	}
	fclose(f);
}


int main()
{
	int i, j;
	simplex_t s;
	lstInit(&s.lstEquacoes);
	int option;

	do {
		printf("-MENU-\n");
		printf("1 - Entrada manual\n");
		printf("2 - Entrada via arquivo\n");
		printf("3 - Sair\n");
		printf("Entre com a opcao: ");
		scanf("%d", &option);

		switch (option)
		{
		case 1:
			printf("Quantas variaveis (maximo 20)? ");
			scanf("%d", &s.qtdVar);
			printf("Quantas restricoes (maximo 20)? ");
			scanf("%d", &s.qtdRest);
			printf("Tipo de problema (Max/Min): ");
			char tpProblema[4];
			scanf("%s[^\n]", &tpProblema);
			int restricoes = s.qtdRest;
			for (i = 0; i < restricoes; i++)
			{
				eqSimplex_t eq;
				printf("Restricao %d\n", i + 1);
				for (j = 0; j < s.qtdVar; j++)
				{	
					printf("Coeficiente  da variavel x%d: ", j);
					scanf("%lf", &eq.x[j]);
				}
				printf("Tipo de restricao: ");
				scanf("%s[^\n]", eq.tp);
				printf("Termo independente: ");
				scanf("%lf", &eq.termo);
				adjustsInsertEq(&s, &eq);
			}
			
			eqSimplex_t objetivo;
			printf("Funcao objetivo\n");
			for (i = 0; i < s.qtdVar; i++)
			{
				printf("Coeficiente  da variavel x%d: ", i);
				scanf("%lf", &objetivo.x[i]);
			}

			if (strcmp(tpProblema, "Max") == 0)
			{
				for (i = 0; i < s.qtdVar; i++) objetivo.x[i] *= -1;
			}

			strcpy(objetivo.tp, "<=");
			objetivo.termo = 0;
			adjustsInsertEq(&s, &objetivo);


			printf("Deseja salvar essa entrada em um arquivo externo (S/N)? ");
			scanf("%s[^\n]", tpProblema);
			if (tpProblema[0] == 'S')
			{
				printf("Entre com o nome do arquivo: ");
				char fName[255];
				scanf("%s[^\n]", fName);
				saveFileB(s, fName);
			}

			// resolução
			addFolga(&s); // adiciona variaveis de folga

			simplexPrint(s);
			printf("\n");
			simplexSolve(&s);
			printf("\n");
			simplexPrint(s);
			printf("\n");

			lstFree(&s.lstEquacoes); // limpa memoria alocada

			break;
		case 2:
			printf("Entre com o nome do arquivo: ");
			char fName[255];
			scanf("%s[^\n]", fName);
			loadFileB(&s, fName);

			addFolga(&s); // adiciona variaveis de folga

			simplexPrint(s);
			printf("\n");
			simplexSolve(&s);
			printf("\n");
			simplexPrint(s);
			printf("\n");

			lstFree(&s.lstEquacoes); // limpa memoria alocada
			break;
		case 3:
			break;
		default:
			printf("Opcao invalida!\n");
			break;
		}
	} while (option != 3);

	
	/*eqSimplex_t eqRest1 = { {10.0, 200.0}, "==", 10000.0 };
	eqSimplex_t eqRest2 = { { 1.0, -5.0 }, ">=", 0.0 };
	eqSimplex_t eqRest3 = { { 1.0, 0.0}, "<=", 400.0 };
	eqSimplex_t eqObjetivo = { { -1.0, -25.0 }, "<=", 00.0 };
	
	adjustsInsertEq(&s, &eqRest1); // ajusta equacoes e adiciona na tabela
	adjustsInsertEq(&s, &eqRest2);
	adjustsInsertEq(&s, &eqRest3);
	adjustsInsertEq	(&s, &eqObjetivo);*/

/*	saveFileB(s, "simplex.bin");
	lstFree(&s.lstEquacoes);
	lstInit(&s.lstEquacoes);
	loadFileB(&s, "simplex.bin");*/


	system("pause");
	return 0;
}