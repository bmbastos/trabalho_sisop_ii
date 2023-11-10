# Compilador a ser usado
CC = gcc

# Opções de compilação
CFLAGS = -Wall -Wextra

# Nome do executável
EXECUTABLE = dropbox

# Listagem de arquivos fonte
SOURCES = main.c server.c client.c

# Objetos gerados
OBJECTS = main.o functions.o server.o client.o

# Dependências para construção do executável
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Alvo padrão
all: $(EXECUTABLE)

# Regra genérica para compilar arquivos .c
%.o: %.c functions.h
	$(CC) $(CFLAGS) -c $<

# Alvo para execução do programa
run: $(EXECUTABLE)
	./$(EXECUTABLE)

# Alvo para limpar arquivos gerados
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)