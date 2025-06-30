#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

#define SEED    0x12345678
#define SEED2   0x87654321
typedef enum {
    HASH_SIMPLE,
    HASH_DOUBLE
} hash_type_t;
typedef struct {
    char cep[6];        // 5 dígitos + '\0'
    char cidade[100];
    char estado[3];     // 2 caracteres + '\0'
} tcep;
typedef struct {
    uintptr_t *table;
    int size;           // número de elementos inseridos
    int max;            // tamanho atual da tabela
    int initial_size;   // tamanho inicial da tabela
    float load_factor;  // taxa de ocupação máxima
    uintptr_t deleted;
    hash_type_t hash_type;
    char *(*get_key)(void *);
} thash;
// Função hash principal 
uint32_t hashf(const char* str, uint32_t h) {
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}
// Segunda função hash para hash duplo
uint32_t hashf2(const char* str) {
    uint32_t h = hashf(str, SEED2);
    // Garantir que seja ímpar para melhor distribuição
    return (h % 7) + 1;
}

// Função para obter a chave (primeiros 5 dígitos do CEP)
char* get_cep_key(void* reg) {
    return ((tcep*)reg)->cep;
}

// Função para alocar estrutura CEP
void* aloca_cep(const char* cep, const char* cidade, const char* estado) {
    tcep* novo_cep = malloc(sizeof(tcep));
    if (novo_cep) {
        strncpy(novo_cep->cep, cep, 5);
        novo_cep->cep[5] = '\0';
        strncpy(novo_cep->cidade, cidade, 99);
        novo_cep->cidade[99] = '\0';
        strncpy(novo_cep->estado, estado, 2);
        novo_cep->estado[2] = '\0';
    }
    return novo_cep;
}

// Função para redimensionar a tabela hash
int hash_resize(thash* h) {
    int old_max = h->max;
    uintptr_t* old_table = h->table;
    
    // Dobrar o tamanho da tabela
    h->max = old_max * 2;
    h->table = calloc(sizeof(uintptr_t), h->max);
    if (!h->table) {
        h->table = old_table;
        h->max = old_max;
        return EXIT_FAILURE;
    }
    
    int old_size = h->size;
    h->size = 0;
    
    // Reinserir todos os elementos
    for (int i = 0; i < old_max; i++) {
        if (old_table[i] != 0 && old_table[i] != h->deleted) {
            void* bucket = (void*)old_table[i];
            uint32_t hash1 = hashf(h->get_key(bucket), SEED);
            int pos = hash1 % h->max;
            
            if (h->hash_type == HASH_DOUBLE) {
                uint32_t hash2 = hashf2(h->get_key(bucket));
                int step = hash2;
                while (h->table[pos] != 0) {
                    pos = (pos + step) % h->max;
                }
            } else {
                while (h->table[pos] != 0) {
                    pos = (pos + 1) % h->max;
                }
            }
            
            h->table[pos] = (uintptr_t)bucket;
            h->size++;
        }
    }
    
    free(old_table);
    return EXIT_SUCCESS;
}

// Construtor da tabela hash
int hash_constroi(thash* h, int nbuckets, float load_factor, hash_type_t type, char* (*get_key)(void*)) {
    h->table = calloc(sizeof(uintptr_t), nbuckets);
    if (h->table == NULL) {
        return EXIT_FAILURE;
    }
    h->max = nbuckets;
    h->initial_size = nbuckets;
    h->size = 0;
    h->load_factor = load_factor;
    h->deleted = (uintptr_t)&(h->size);
    h->hash_type = type;
    h->get_key = get_key;
    return EXIT_SUCCESS;
}

// Inserção na tabela hash
int hash_insere(thash* h, void* bucket) {
    // Verificar se precisa redimensionar
    float current_load = (float)h->size / h->max;
    if (current_load >= h->load_factor) {
        if (hash_resize(h) != EXIT_SUCCESS) {
            free(bucket);
            return EXIT_FAILURE;
        }
    }
    
    uint32_t hash1 = hashf(h->get_key(bucket), SEED);
    int pos = hash1 % h->max;
    
    if (h->hash_type == HASH_DOUBLE) {
        uint32_t hash2 = hashf2(h->get_key(bucket));
        int step = hash2;
        
        while (h->table[pos] != 0 && h->table[pos] != h->deleted) {
            pos = (pos + step) % h->max;
        }
    } else {
        while (h->table[pos] != 0 && h->table[pos] != h->deleted) {
            pos = (pos + 1) % h->max;
        }
    }
    
    h->table[pos] = (uintptr_t)bucket;
    h->size++;
    return EXIT_SUCCESS;
}

// Busca na tabela hash
void* hash_busca(thash h, const char* key) {
    uint32_t hash1 = hashf(key, SEED);
    int pos = hash1 % h.max;
    
    if (h.hash_type == HASH_DOUBLE) {
        uint32_t hash2 = hashf2(key);
        int step = hash2;
        
        while (h.table[pos] != 0) {
            if (h.table[pos] != h.deleted && 
                strcmp(h.get_key((void*)h.table[pos]), key) == 0) {
                return (void*)h.table[pos];
            }
            pos = (pos + step) % h.max;
        }
    } else {
        while (h.table[pos] != 0) {
            if (h.table[pos] != h.deleted && 
                strcmp(h.get_key((void*)h.table[pos]), key) == 0) {
                return (void*)h.table[pos];
            }
            pos = (pos + 1) % h.max;
        }
    }
    return NULL;
}

// Remoção da tabela hash
int hash_remove(thash* h, const char* key) {
    uint32_t hash1 = hashf(key, SEED);
    int pos = hash1 % h->max;
    
    if (h->hash_type == HASH_DOUBLE) {
        uint32_t hash2 = hashf2(key);
        int step = hash2;
        
        while (h->table[pos] != 0) {
            if (h->table[pos] != h->deleted && 
                strcmp(h->get_key((void*)h->table[pos]), key) == 0) {
                free((void*)h->table[pos]);
                h->table[pos] = h->deleted;
                h->size--;
                return EXIT_SUCCESS;
            }
            pos = (pos + step) % h->max;
        }
    } else {
        while (h->table[pos] != 0) {
            if (h->table[pos] != h->deleted && 
                strcmp(h->get_key((void*)h->table[pos]), key) == 0) {
                free((void*)h->table[pos]);
                h->table[pos] = h->deleted;
                h->size--;
                return EXIT_SUCCESS;
            }
            pos = (pos + 1) % h->max;
        }
    }
    return EXIT_FAILURE;
}

// Destrutor da tabela hash
void hash_apaga(thash* h) {
    for (int pos = 0; pos < h->max; pos++) {
        if (h->table[pos] != 0 && h->table[pos] != h->deleted) {
            free((void*)h->table[pos]);
        }
    }
    free(h->table);
    h->table = NULL;
    h->size = 0;
    h->max = 0;
}

// Função para obter taxa de ocupação atual
float hash_get_load_factor(thash* h) {
    return (float)h->size / h->max;
}

// Função para carregar CEPs de arquivo CSV
int carregar_ceps_csv(thash* h, const char* filename, int max_records) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Erro ao abrir arquivo %s\n", filename);
        return EXIT_FAILURE;
    }
    
    char line[1000];
    char cep[10], cidade[100], estado[10];
    int count = 0;
    
    // Pular cabeçalho se existir
    fgets(line, sizeof(line), file);
    
    while (fgets(line, sizeof(line), file) && count < max_records) {
        // Assumindo formato: CEP,CIDADE,ESTADO
        if (sscanf(line, "%[^,],%[^,],%s", cep, cidade, estado) == 3) {
            // Extrair apenas os primeiros 5 dígitos do CEP
            char cep5[6];
            strncpy(cep5, cep, 5);
            cep5[5] = '\0';
            
            void* novo_cep = aloca_cep(cep5, cidade, estado);
            if (novo_cep) {
                if (hash_insere(h, novo_cep) == EXIT_SUCCESS) {
                    count++;
                } else {
                    free(novo_cep);
                }
            }
        }
    }
    
    fclose(file);
    return count;
}

// Funções específicas para testes de performance com diferentes taxas de ocupação
void busca10(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca20(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca30(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca40(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca50(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca60(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca70(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca80(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca90(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

void busca99(thash* h, char keys[][6], int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        hash_busca(*h, keys[i]);
    }
}

// Funções específicas para teste de inserção
void insere6100(const char* filename) {
    thash h;
    hash_constroi(&h, 6100, 0.75, HASH_SIMPLE, get_cep_key);
    carregar_ceps_csv(&h, filename, 50000);
    hash_apaga(&h);
}

void insere1000(const char* filename) {
    thash h;
    hash_constroi(&h, 1000, 0.75, HASH_SIMPLE, get_cep_key);
    carregar_ceps_csv(&h, filename, 50000);
    hash_apaga(&h);
}

// Função para medir tempo
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Função para testar diferentes taxas de ocupação
void teste_taxa_ocupacao(const char* filename) {
    printf("Taxa\tHash Simples (s)\tHash Duplo (s)\n");
    
    float taxas[] = {0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.99};
    int num_taxas = sizeof(taxas) / sizeof(taxas[0]);
    
    for (int i = 0; i < num_taxas; i++) {
        thash h_simple, h_double;
        int target_size = (int)(6100 * taxas[i]);
        
        // Teste hash simples
        hash_constroi(&h_simple, 6100, 1.0, HASH_SIMPLE, get_cep_key);
        carregar_ceps_csv(&h_simple, filename, target_size);
        
        // Teste hash duplo
        hash_constroi(&h_double, 6100, 1.0, HASH_DOUBLE, get_cep_key);
        carregar_ceps_csv(&h_double, filename, target_size);
        
        // Preparar chaves para busca
        char keys[1000][6];
        int key_count = 0;
        for (int j = 0; j < h_simple.max && key_count < 1000; j++) {
            if (h_simple.table[j] != 0 && h_simple.table[j] != h_simple.deleted) {
                tcep* cep = (tcep*)h_simple.table[j];
                strcpy(keys[key_count], cep->cep);
                key_count++;
            }
        }
        
        // Medir tempo de busca - Hash Simples
        double start = get_time();
        for (int rep = 0; rep < 1000; rep++) {
            for (int k = 0; k < key_count; k++) {
                hash_busca(h_simple, keys[k]);
            }
        }
        double time_simple = get_time() - start;
        
        // Medir tempo de busca - Hash Duplo
        start = get_time();
        for (int rep = 0; rep < 1000; rep++) {
            for (int k = 0; k < key_count; k++) {
                hash_busca(h_double, keys[k]);
            }
        }
        double time_double = get_time() - start;
        
        printf("%.0f%%\t%.6f\t\t%.6f\n", taxas[i] * 100, time_simple, time_double);
        
        hash_apaga(&h_simple);
        hash_apaga(&h_double);
    }
}

// Função para testar overhead de inserção
void teste_overhead_insercao(const char* filename) {
    printf("\nTestando overhead de inserção...\n");
    
    double start, time_6100, time_1000;
    
    // Teste com 6100 buckets iniciais
    start = get_time();
    insere6100(filename);
    time_6100 = get_time() - start;
    
    // Teste com 1000 buckets iniciais
    start = get_time();
    insere1000(filename);
    time_1000 = get_time() - start;
    
    printf("Tempo para inserir 6100 buckets: %.6f segundos\n", time_6100);
    printf("Tempo para inserir 1000 buckets: %.6f segundos\n", time_1000);
    printf("Overhead estrutura dinâmica: %.2f%%\n", 
           ((time_1000 - time_6100) / time_6100) * 100);
}

// Função principal de teste
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo_ceps.csv>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    const char* filename = argv[1];
    
    // Teste básico de funcionalidade
    thash h;
    hash_constroi(&h, 100, 0.75, HASH_SIMPLE, get_cep_key);
    
    // Inserir alguns CEPs de teste
    hash_insere(&h, aloca_cep("01310", "São Paulo", "SP"));
    hash_insere(&h, aloca_cep("20040", "Rio de Janeiro", "RJ"));
    hash_insere(&h, aloca_cep("30112", "Belo Horizonte", "MG"));
    
    // Testar busca
    tcep* resultado = (tcep*)hash_busca(h, "01310");
    if (resultado) {
        printf("CEP %s encontrado: %s, %s\n", 
               resultado->cep, resultado->cidade, resultado->estado);
    }
    
    hash_apaga(&h);
    
    // Executar testes de performance
    teste_taxa_ocupacao(filename);
    teste_overhead_insercao(filename);
    
    return EXIT_SUCCESS;
}
