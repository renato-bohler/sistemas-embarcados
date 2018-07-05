#define QUEUE_LIMIT 64
#define MAX_MSG_SIZE 16
#define NUM_ANDARES 16
#define TEMPO_PORTA_ABERTA 2000

typedef enum porta
{
  fechada = 1,
  aberta
} porta;

typedef enum estado
{
  esperando_inicializacao = 1,
  standby,
  fechando_porta,
  abrindo_porta,
  subindo,
  descendo
} estado;

typedef enum sentido
{
  elevador_parado = 1,
  elevador_subindo,
  elevador_descendo
} sentido;

// Struct da mensagem a ser trocada entre as tarefas
typedef struct
{
  char conteudo[MAX_MSG_SIZE];
} mensagem;