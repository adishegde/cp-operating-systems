BUILD = build
BIN = bin
CC = gcc

$(BIN)/t_db: $(BUILD)/t_database.o $(BUILD)/database.o $(BUILD)/u_io.o
	$(CC) $^ -o $@

$(BUILD)/u_io.o: utils/io.c utils/io.h
	$(CC) -o $@ -c utils/io.c

$(BUILD)/database.o: database.c database.h utils/io.h utils/commons.h
	$(CC) -o $@ -c database.c

$(BUILD)/t_database.o: test/database.c database.h utils/commons.h
	$(CC) -o $@ -c test/database.c

.PHONY: clean t_db


t_db: $(BIN)/t_db


clean:
	rm -r $(BUILD)

$(shell mkdir -p $(BUILD))
$(shell mkdir -p $(BIN))
