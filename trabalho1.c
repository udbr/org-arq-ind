#ifdef _MSC_VER 
#define _CRT_SECURE_NO_WARNINGS 1
#define strcasecmp _stricmp
#endif
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#define TAM_REG 128
#define TAM_INDICE_NUM 24
#define TAM_INDICE_NOME 80
#define TAM_HASH 34000

typedef struct candidato {
	unsigned long long numero;
	unsigned long long cpf;
	char nome[64];
	char cidade[32];
	unsigned char idade;
	unsigned char genero;
	unsigned char instrucao;
	struct candidato* prox;
} candidato_t;

typedef struct indice_num {
	unsigned long long numero;
	unsigned long pos;
	struct indice_num* prox;
} indice_t;

typedef struct indice_nome {
	char nome[64];
	unsigned long pos;
	struct indice_nome* prox;
} indice_nome_t;

typedef struct indice_hash {
	unsigned long pos;
	struct indice_hash* prox;
} indice_hash_t;

typedef struct indice_arvore {
	unsigned long long numero;
	unsigned long pos;
	struct indice_arvore* esq, * dir;
} indice_arvore_t;

void* raiz = NULL;
indice_hash_t hashtable[TAM_HASH];

/* Substituto para strtok() pq o mesmo nao lida com campo vazio */
char* strsep_w(char** stringp, const char* delim) {
	char* start = *stringp;
	char* p;

	p = (start != NULL) ? strpbrk(start, delim) : NULL;
	if (p == NULL)
		*stringp = NULL;
	else {
		*p = '\0';
		*stringp = p + 1;
	}
	return start;
}

/* Limpa input invalido no scanf */
void limpa_input() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF) {}
}

/* Funcao de hash usando alguns primos */
int hash(unsigned long long cpf) {
	return ((cpf % TAM_HASH) * 7 * 13) % TAM_HASH;
}

/* Cria nodo inicializado com zero */
void* cria_nodo(size_t size) {
	void* novo = malloc(size);
	memset(novo, 0, size);
	return novo;
}

/* Insere nodo em lista ordenada para gravacao em arquivo de dados ou indice */
void insere_ordenado(void* nodo, char tipo) {
	if (!raiz || (!tipo && (((candidato_t*)raiz)->cpf > ((candidato_t*)nodo)->cpf)) ||
		(tipo == 1 && (((indice_t*)raiz)->numero > ((indice_t*)nodo)->numero)) ||
		(tipo == 2 && strcasecmp(((indice_nome_t*)raiz)->nome,((indice_nome_t*)nodo)->nome) > 0)) {
		if (!tipo)
			((candidato_t*)nodo)->prox = raiz;
		else if (tipo == 1)
			((indice_t*)nodo)->prox = raiz;
		else
			((indice_nome_t*)nodo)->prox = raiz;
		raiz = nodo;
		return;
	}
	void* atual = raiz;
	while ((!tipo && ((candidato_t*)atual)->prox != NULL && ((candidato_t*)atual)->prox->cpf < ((candidato_t*)nodo)->cpf) ||
		(tipo == 1 && ((indice_t*)atual)->prox != NULL && ((indice_t*)atual)->prox->numero < ((indice_t*)nodo)->numero) ||
		(tipo == 2 && ((indice_nome_t*)atual)->prox != NULL && strcasecmp(((indice_nome_t*)atual)->prox->nome, ((indice_nome_t*)nodo)->nome) < 0))
		if (!tipo)
			atual = ((candidato_t*)atual)->prox;
		else if (tipo == 1)
			atual = ((indice_t*)atual)->prox;
		else
			atual = ((indice_nome_t*)atual)->prox;
	if (!tipo) {
		((candidato_t*)nodo)->prox = ((candidato_t*)atual)->prox;
		((candidato_t*)atual)->prox = nodo;
	}
	else if (tipo == 1) {
		((indice_t*)nodo)->prox = ((indice_t*)atual)->prox;
		((indice_t*)atual)->prox = nodo;
	}
	else {
		((indice_nome_t*)nodo)->prox = ((indice_nome_t*)atual)->prox;
		((indice_nome_t*)atual)->prox = nodo;
	}
	return;
}

/* Insere nodo em arvore binaria, sem recursao */
void insere_arvore(void** raiz, indice_arvore_t* nodo) {
	if (!*raiz) {
		*raiz = nodo;
		return;
	}
	indice_arvore_t* folha = *raiz; // 33k registros causa buffer overflow, entao sem recursao
	while (folha != NULL) {
		if (nodo->numero < folha->numero) {
			if (!folha->esq) {
				folha->esq = nodo;
				return;
			}
			folha = folha->esq;
			continue;
		}
		else {
			if (!folha->dir) {
				folha->dir = nodo;
				return;
			}
			folha = folha->dir;
		}
	}
	folha = nodo;
}

/* Deleta listas encadeadas temporarias */
void deleta_lista(char tipo) {
	void* atual;
	while (raiz != NULL) {
		atual = raiz;
		if (!tipo)
			(candidato_t*)raiz = ((candidato_t*)raiz)->prox;
		else if (tipo < 2)
			(indice_t*)raiz = ((indice_t*)raiz)->prox;
		else
			(indice_nome_t*)raiz = ((indice_nome_t*)raiz)->prox;
		free(atual);
	}
	atual = raiz = NULL;
}

/* Deleta arvore binaria */
void deleta_arvore(indice_arvore_t* raiz) {
	if (raiz == NULL) return;
	deleta_arvore(raiz->esq);
	deleta_arvore(raiz->dir);
	free(raiz);
}

/* Deleta encadeamentos da tabela hash */
void deleta_tabela_hash(indice_hash_t *raiz) {
	if (raiz == NULL) return;
	deleta_tabela_hash(raiz->prox);
	free(raiz);
}

/* Pula campos do arquivo de origem */
void pula_campos(int num, char** string) {
	int i;
	for (i = 0; i < num; i++)
		strsep_w(string, "\"");
}

/* Processa arquivo de origem e extrai campos para arquivo binario */
void cria_arquivo_dados(FILE* origem, FILE* dados) {
	char* str, linha[1024];
	while (fgets(linha, 1024, origem) != NULL) {
		candidato_t* novo = cria_nodo(sizeof(candidato_t));
		str = linha;
		pula_campos(25, &str);
		strcpy(novo->cidade, strsep_w(&str, "\""));
		pula_campos(5, &str);
		sscanf(strsep_w(&str, "\""), "%llu", &novo->numero);
		pula_campos(3, &str);
		strcpy(novo->nome, strsep_w(&str, "\""));
		pula_campos(5, &str);
		sscanf(strsep_w(&str, "\""), "%llu", &novo->cpf);
		pula_campos(37, &str);
		sscanf(strsep_w(&str, "\""), "%hhu", &novo->idade);
		pula_campos(3, &str);
		sscanf(strsep_w(&str, "\""), "%c", &novo->genero);
		pula_campos(3, &str);
		sscanf(strsep_w(&str, "\""), "%c", &novo->instrucao);
		insere_ordenado(novo, 0);
	}
	candidato_t* lista = raiz;
	while (lista != NULL) {
		fwrite(lista, sizeof(candidato_t), 1, dados);
		lista = lista->prox;
	}
}

/* Cria arquivos binario de indice a partir dos dados */
void cria_arquivo_indice(FILE* dados, FILE* indice, int tipo) {
	int count = 0;
	long pos;
	void* novo;
	candidato_t* candidato = malloc(sizeof(candidato_t));
	rewind(dados);
	pos = ftell(dados);
	while (fread(candidato, sizeof(candidato_t), 1, dados) > 0) {
		if (tipo < 2) {
			novo = cria_nodo(sizeof(indice_t));
			((indice_t*)novo)->numero = candidato->numero;
		}
		else {
			novo = cria_nodo(sizeof(indice_nome_t));
			memcpy(((indice_nome_t*)novo)->nome, candidato->nome, sizeof(char) * 64);
		}
		if (tipo < 2) ((indice_t*)novo)->pos = pos;
		else ((indice_nome_t*)novo)->pos = pos;
		insere_ordenado(novo, tipo);
		pos = ftell(dados);
	}
	free(candidato);
	void* lista = raiz;
	while (lista != NULL) {
		if (tipo < 2) {
			fwrite(lista, sizeof(indice_t), 1, indice);
			lista = ((indice_t*)lista)->prox;
			count++;
		}
		else {
			fwrite(lista, sizeof(indice_nome_t), 1, indice);
			lista = ((indice_nome_t*)lista)->prox;
		}
	}
}

/* Cria tabela hash */
cria_hash_table(FILE* dados) {
	int i, pos, chave;
	candidato_t* candidato = cria_nodo(sizeof(candidato_t));
	indice_hash_t* lista;

	for (i = 0; i < TAM_HASH; i++) {
		hashtable[i].pos = -1;
		hashtable[i].prox = NULL;
	}
	rewind(dados);
	pos = ftell(dados);
	while (fread(candidato, sizeof(candidato_t), 1, dados) > 0) {
		chave = hash(candidato->cpf);
		if (hashtable[chave].pos == -1) {
			hashtable[chave].pos = pos;
			pos = ftell(dados);
			continue;
		}
		indice_hash_t* novo = cria_nodo(sizeof(indice_hash_t));
		novo->pos = pos;
		if (hashtable[chave].prox == NULL) {
			hashtable[chave].prox = novo;
			continue;
		}
		lista = &hashtable[chave];
		while (lista->prox != NULL)
			lista = lista->prox;
		lista->prox = novo;
		pos = ftell(dados);
	}
	free(candidato);
}

/* Cria arvore binaria para consulta de dados */
void cria_arvore_binaria(FILE *dados) {
	int pos;
	candidato_t* candidato = cria_nodo(sizeof(candidato_t));
	rewind(dados);
	pos = ftell(dados);
	while (fread(candidato, sizeof(candidato_t), 1, dados) > 0) {
		indice_arvore_t* novo = cria_nodo(sizeof(indice_arvore_t));
		novo->numero = candidato->numero;
		novo->pos = pos;
		insere_arvore(&raiz, novo);
		pos = ftell(dados);
	}
	free(candidato);
}

/* Converte codigo de genero para letra */
char mostra_genero(char genero) {
	return (genero == '2') ? 'M' : 'F';
}

/* Converte codigo escolaridade em string */
char* mostra_escolaridade(char codigo) {
	switch (codigo) {
	case '2':
		return "LE E ESCREVE";
	case '3':
		return "FUNDAMENTAL INCOMPLETO";
	case '4':
		return "FUNDAMENTAL COMPLETO";
	case '5':
		return "MEDIO INCOMPLETO";
	case '6':
		return "MEDIO COMPLETO";
	case '7':
		return "SUPERIOR INCOMPLETO";
	case '8':
		return "SUPERIOR COMPLETO";
	default:
		return "NULO";
	}
}

/* Percorre todo o arquivo de dados escrevendo os dados na tela */
void mostra_dados(FILE* arquivo) {
	candidato_t* candidato = malloc(sizeof(candidato_t));
	rewind(arquivo);
	while (fread(candidato, sizeof(candidato_t), 1, arquivo) > 0) {
		printf("Cand Num: %llu, Nome: %s, CPF: %llu, Idade: %d, Genero: %c, Instrucao: %s, Cidade: %s\n",
			candidato->numero, candidato->nome, candidato->cpf, candidato->idade, mostra_genero(candidato->genero),
			mostra_escolaridade(candidato->instrucao), candidato->cidade);
	}
	free(candidato);
}

/* Busca registro em arquivo de dados baseado em posicao */
void le_registro_dados(FILE *arquivo, int pos) {
	candidato_t* candidato = malloc(sizeof(candidato_t));
	fseek(arquivo, pos, SEEK_SET);
	fread(candidato, sizeof(candidato_t), 1, arquivo);
	printf("\nEncontrado! Cand Num: %llu, Nome: %s, CPF: %llu, Idade: %d, Genero: %c, Instrucao: %s, Cidade: %s\n\n",
		candidato->numero, candidato->nome, candidato->cpf, candidato->idade, mostra_genero(candidato->genero),
		mostra_escolaridade(candidato->instrucao), candidato->cidade);
	free(candidato);
	return;
}

/* Busca posicao em arquivo de index e entao busca registro no arquivo de dados */
void le_registro_indice(FILE* dados, FILE* indice, int pos, int tipo) {
	void* idx;
	candidato_t* candidato = cria_nodo(sizeof(candidato_t));
	fseek(indice, pos, SEEK_SET);
	if (tipo == 1) {
		idx = cria_nodo(sizeof(indice_t));
		fread(idx, sizeof(indice_t), 1, indice);
		fseek(dados, ((indice_t *)idx)->pos, SEEK_SET);
	}
	else {
		idx = cria_nodo(sizeof(indice_nome_t));
		fread(idx, sizeof(indice_nome_t), 1, indice);
		fseek(dados, ((indice_nome_t*)idx)->pos, SEEK_SET);
	}
	fread(candidato, sizeof(candidato_t), 1, dados);
	printf("\nEncontrado! Cand Num: %llu, Nome: %s, CPF: %llu, Idade: %d, Genero: %c, Instrucao: %s, Cidade: %s\n\n",
		candidato->numero, candidato->nome, candidato->cpf, candidato->idade, mostra_genero(candidato->genero),
		mostra_escolaridade(candidato->instrucao), candidato->cidade);
	free(candidato);
	free(idx);
}

/* Pesquisa binaria no arquivo de dados ou indice */
void pesquisa_binaria(FILE* dados, FILE *indice, unsigned long long num, char *nome, int inicio, int fim, int tamanho, int offset, int tipo) {
	int meio;
	unsigned long long chave_num;
	void* arquivo;
	char chave_nome[64];
	if (!tipo) arquivo = dados;
	else arquivo = indice;
	while (inicio <= fim) {
		meio = (inicio + fim) / 2;
		fseek(arquivo, (meio * tamanho) + offset, SEEK_SET);
		if (tipo < 2)
			fread(&chave_num, sizeof(long long), 1, arquivo);
		else
			fread(chave_nome, sizeof(char), 64, arquivo);
		if ((tipo < 2 && num == chave_num) || (tipo == 2 && strcasecmp(nome, chave_nome) == 0)) {
			if (!tipo)
				le_registro_dados(dados, meio * tamanho);
			else
				le_registro_indice(dados, indice, meio * tamanho, tipo);
			return;
		}
		else if ((tipo < 2 && chave_num < num) || (tipo == 2 && strcasecmp(chave_nome, nome) < 0)) {
			inicio = meio + 1;
		}
		else {
			fim = meio - 1;
		}
	}
	printf("\nRegistro nao encontrado!\n\n");
}

/* Pesquisa de registro em tabela hash */
void pesquisa_hash(FILE* dados, unsigned long long cpf) {
	indice_hash_t* lista;
	candidato_t* candidato = cria_nodo(sizeof(candidato_t));
	lista = &hashtable[hash(cpf)];
	while (lista != NULL && lista->pos != -1) {
		fseek(dados, lista->pos, SEEK_SET);
		fread(candidato, sizeof(candidato_t), 1, dados);
		if (candidato->cpf == cpf) {
			le_registro_dados(dados, lista->pos);
			free(candidato);
			return;
		}
		lista = lista->prox;
	}
	printf("\nRegistro nao encontrado!\n\n");
	free(candidato);
}

/* Pesquisa de registro em arvore binaria */
void pesquisa_arvore(FILE *dados, void* raiz, unsigned long long numero) {
	if ((indice_arvore_t*)raiz == NULL) {
		printf("\nRegistro nao encontrado!\n\n");
		return;
	}
	if (((indice_arvore_t*)raiz)->numero == numero) {
		le_registro_dados(dados, ((indice_arvore_t*)raiz)->pos);
		return;
	}
	if (numero < ((indice_arvore_t*)raiz)->numero) {
		pesquisa_arvore(dados, ((indice_arvore_t*)raiz)->esq, numero);
		return;
	}
	else {
		pesquisa_arvore(dados, ((indice_arvore_t*)raiz)->dir, numero);
		return;
	}
}

/* Calcula dados de genero para hipotese */
void calcula_genero(FILE* dados) {
	int h = 0, m = 0;
	candidato_t* candidato = malloc(sizeof(candidato_t));
	rewind(dados);
	while (fread(candidato, sizeof(candidato_t), 1, dados) > 0) {
		if (candidato->genero == '2')
			h++;
		else if (candidato->genero == '4')
			m++;
	}
	printf("\n %d (%.1f%%) candidatos homens para %d (%.1f%%) candidatas mulheres de um total de %d candidatos\n\n",
			h, ((double)h / (h + m)) * 100, m, ((double)m / (h + m)) * 100, h + m);
	free(candidato);
}

/* Calcula dados de idade para hipotese */
void calcula_idade(FILE* dados) {
	int idade = 0, contador = 0;
	candidato_t* candidato = malloc(sizeof(candidato_t));
	rewind(dados);
	while (fread(candidato, sizeof(candidato_t), 1, dados) > 0) {
		idade += candidato->idade;
		contador++;
	}
	printf("\n A idade media dos candidatos eh %1.f anos\n\n", (double)idade / contador);
	free(candidato);
}

/* Calcula dados de instrucao para hipotese */
void calcula_instrucao(FILE* dados) {
	int instrucao[7] = { 0 }, total = 0;
	candidato_t* candidato = malloc(sizeof(candidato_t));
	rewind(dados);
	while (fread(candidato, sizeof(candidato_t), 1, dados) > 0) {
		instrucao[(candidato->instrucao - '0') - 2]++;
		total++;
	}
	printf("\n Instrucao dos candidatos por nivel escolar:\n\n");
	printf("\t%24s %d\t(%.1f%%)\n\t%24s %d\t(%.1f%%)\n", mostra_escolaridade('2'), instrucao[0], ((double)instrucao[0] / total) * 100, mostra_escolaridade('3'), instrucao[1], ((double)instrucao[1] / total) * 100);
	printf("\t%24s %d\t(%.1f%%)\n\t%24s %d\t(%.1f%%)\n", mostra_escolaridade('4'), instrucao[2], ((double)instrucao[2] / total) * 100, mostra_escolaridade('5'), instrucao[3], ((double)instrucao[3] / total) * 100);
	printf("\t%24s %d\t(%.1f%%)\n\t%24s %d\t(%.1f%%)\n", mostra_escolaridade('6'), instrucao[4], ((double)instrucao[4] / total) * 100, mostra_escolaridade('7'), instrucao[5], ((double)instrucao[5] / total) * 100);
	printf("\t%24s %d\t(%.1f%%)\n\n", mostra_escolaridade('8'), instrucao[6], ((double)instrucao[6] / total) * 100);
	free(candidato);
}

/* Menu de opcoes */
void mostra_menu() {
	printf("\n\tTDE 1: Organizacao de Arquivos e Indices\n\n");
	printf("[1] Mostrar dados do arquivo de dados\n");
	printf("[2] Pesquisa binaria por CPF no arquivo de dados\n");
	printf("[3] Pesquisa binaria por Numero do Candidato no arquivo de indice\n");
	printf("[4] Pesquisa binaria por Nome no arquivo de indice\n");
	printf("[5] Pesquisa hash em memoria por CPF\n");
	printf("[6] Pesquisa arvore binaria em memoria por Numero do Candidato\n");
	printf("[7] Hipotese 1: Numero de candidatos mulheres aumentou desde a ultima eleicao\n");
	printf("[8] Hipotese 2: Idade media dos candidatos diminuiu desde a ultima eleicao\n");
	printf("[9] Hipotese 3: Grau de instrucao dos candidatos aumentou desde a ultima eleicao\n");
	printf("[0] Sair\n\n");
	printf("Selecao: ");
}

int main() {
	FILE* origem, * dados, * indice_num, * indice_nome;
	int i, opcao;
	unsigned long long llbuffer;
	char nome[64];


	origem = fopen("consulta_cand_2020_RS.csv", "r");
	dados = fopen("dados.dat", "wb+");
	indice_num = fopen("indice_num.dat", "wb+");
	indice_nome = fopen("indice_nome.dat", "wb+");
	if (!origem || !dados || !indice_num || !indice_nome) {
		printf("Falha ao abrir origem ou dados ou indice\n");
		return 1;
	}
	printf("Criando arquivos de dados e indices, aguarde...");
	cria_arquivo_dados(origem, dados);
	cria_hash_table(dados);
	deleta_lista(0);
	cria_arquivo_indice(dados, indice_num, 1);
	deleta_lista(1);
	cria_arquivo_indice(dados, indice_nome, 2);
	deleta_lista(2);
	cria_arvore_binaria(dados);

	fclose(origem);

	while (1) {
		system("cls");
		mostra_menu();
		scanf("%d", &opcao);
		switch (opcao)
		{
		case 1:
			mostra_dados(dados);
			system("pause");
			break;
		case 2:
			printf("CPF (somente numeros): ");
			limpa_input();
			scanf("%llu", &llbuffer);
			limpa_input();
			fseek(dados, 0, SEEK_END);
			system("cls");
			pesquisa_binaria(dados, NULL, llbuffer, NULL, 0, (int)(ftell(dados) / TAM_REG) - 1, TAM_REG, 8, 0);
			system("pause");
			break;
		case 3:
			printf("Numero: ");
			limpa_input();
			scanf("%llu", &llbuffer);
			limpa_input();
			fseek(indice_num, 0, SEEK_END);
			system("cls");
			pesquisa_binaria(dados, indice_num, llbuffer, NULL, 0, (int)(ftell(indice_num) / TAM_INDICE_NUM) - 1, TAM_INDICE_NUM, 0, 1);
			system("pause");
			break;
		case 4:
			printf("Nome completo (case insensitive): ");
			limpa_input();
			scanf("%63[^\n]", nome);
			limpa_input();
			fseek(indice_nome, 0, SEEK_END);
			system("cls");
			pesquisa_binaria(dados, indice_nome, 0, nome, 0, (int)(ftell(indice_nome) / TAM_INDICE_NOME) - 1, TAM_INDICE_NOME, 0, 2);
			system("pause");
			break;
		case 5:
			printf("CPF (somente numeros): ");
			limpa_input();
			scanf("%llu", &llbuffer);
			limpa_input();
			system("cls");
			pesquisa_hash(dados, llbuffer);
			system("pause");
			break;
		case 6:
			printf("Numero: ");
			limpa_input();
			scanf("%llu", &llbuffer);
			limpa_input();
			system("cls");
			pesquisa_arvore(dados, raiz, llbuffer);
			system("pause");
			break;
		case 7:
			system("cls");
			calcula_genero(dados);
			system("pause");
			break;
		case 8:
			system("cls");
			calcula_idade(dados);
			system("pause");
			break;
		case 9:
			system("cls");
			calcula_instrucao(dados);
			system("pause");
			break;
		case 0:
			for (i = 0; i < TAM_HASH; i++) {
				deleta_tabela_hash(hashtable[i].prox);
			}
			deleta_arvore(raiz);
			fclose(dados);
			fclose(indice_num);
			fclose(indice_nome);
			return 0;
		default:
			break;
		}
	}
	return 0;
}