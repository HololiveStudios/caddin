CC ?= gcc
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -lm

SRCS = pestinha.c neuro.c homeostasis.c behavior.c conditioning.c memory.c sensors.c terminal_ui.c
OBJS = $(SRCS:.c=.o)
TARGET = pestinha_term

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build concluida com sucesso: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
