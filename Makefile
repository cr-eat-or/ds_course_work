# Сборка в build/: консоль «Жизнь» + test.exe (юнит-тесты).
# Отладка: make DEBUG=1 all
# Очистка: make clean

CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS = -Iinclude -Itests/include
LDFLAGS  =

ifeq ($(DEBUG),1)
CFLAGS += -g -O0
endif

BUILD = build

# Имя консольного бинарника: на Windows — .exe, на Unix — без суффикса.
ifeq ($(OS),Windows_NT)
MAIN = $(BUILD)/main.exe
else
MAIN = $(BUILD)/main
endif

# Ядро курсовой (без консоли): общее для main и test.exe.
CORE = src/grid.c src/life.c src/cycle.c src/templates.c
# Все юнит-тесты (точка входа — test_main.c).
TEST = tests/unit/test_main.c tests/unit/test_grid.c tests/unit/test_life.c \
       tests/unit/test_cycle.c tests/unit/test_templates.c

.PHONY: all clean test tests main

all: $(MAIN) $(BUILD)/test.exe

$(BUILD):
	mkdir -p $(BUILD)

$(MAIN): src/main.c $(CORE) | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ src/main.c $(CORE) $(LDFLAGS)

main: $(MAIN)

$(BUILD)/test.exe: $(TEST) $(CORE) | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(TEST) $(CORE) $(LDFLAGS)

test: $(BUILD)/test.exe
tests: test

ifeq ($(OS),Windows_NT)
clean:
	cmd /C "if exist $(BUILD) rd /s /q $(BUILD)"
else
clean:
	rm -rf $(BUILD)
endif
