# Компилятор, которым собираем проект
CC=gcc

# Стандарт языка C: используем C11
CSTD=-std=c11

# Флаги предупреждений:
# -Wall      включает основные предупреждения
# -Wextra    включает дополнительные предупреждения
# -Wpedantic проверяет строгое соответствие стандарту C
WARN=-Wall -Wextra -Wpedantic

# Отладочные флаги:
# -g добавляет отладочную информацию
# -fno-omit-frame-pointer помогает получать нормальные stack trace
DBG=-g -fno-omit-frame-pointer

# Sanitizers:
# address   ловит ошибки памяти
# undefined ловит неопределённое поведение
SAN=-fsanitize=address,undefined

# Пути, где компилятор будет искать заголовочные файлы .h
INC=-Iinclude -Isrc

# Основные исходные файлы проекта.
# Здесь перечислены реализации алгоритмов и вспомогательные структуры.
SRC=src/naive.c \
	src/tarjan.c \
	src/gabow.c \
	src/reachability.c \
	src/dsu.c \
	src/fib_heap.c

# Общий набор флагов для debug-сборки и тестов
COMMON=$(CSTD) $(WARN) $(DBG)

# .PHONY говорит make, что это не имена файлов, а команды.
# Например, даже если появится файл clean, make clean всё равно выполнится.
.PHONY: all release debug test clean

# Цель по умолчанию.
# Если написать просто make, будет выполнен make release.
all: release

# Release-сборка:
# собирает оптимизированную версию программы dmst.
release:
	$(CC) $(CSTD) -O2 -DNDEBUG \
	$(WARN) $(INC) \
	$(SRC) src/main.c \
	-o dmst

# Debug-сборка:
# собирает программу с отладочной информацией.
debug:
	$(CC) $(COMMON) $(INC) \
	$(SRC) src/main.c \
	-o dmst

# Сборка тестов:
# собирает отдельный исполняемый файл tests_run
# вместе с AddressSanitizer и UndefinedBehaviorSanitizer.
tests_run:
	$(CC) $(COMMON) $(SAN) \
	$(INC) $(SRC) tests/test.c \
	$(SAN) -o tests_run

# Запуск тестов:
# сначала собирается tests_run, потом он запускается.
test: tests_run
	./tests_run

# Очистка результатов сборки.
# -f означает: не ругаться, если файла нет.
clean:
	rm -f dmst tests_run
