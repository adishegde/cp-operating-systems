BUILD = build
BIN = bin
CC = gcc

$(BIN)/t_db: $(BUILD)/t_database.o $(BUILD)/database.o $(BUILD)/u_io.o
	$(CC) -o $@ -pthread $^

$(BIN)/server: $(BUILD)/server.o $(BUILD)/database.o $(BUILD)/u_io.o
	$(CC) -o $@ -pthread $^

$(BIN)/client: $(BUILD)/client.o $(BUILD)/u_io.o $(BUILD)/u_fort.o
	$(CC) -o $@ $^

$(BUILD)/u_io.o: lib/io.c lib/io.h
	$(CC) -o $@ -c lib/io.c

$(BUILD)/database.o: lib/database.c lib/database.h lib/io.h lib/commons.h
	$(CC) -o $@ -c lib/database.c

$(BUILD)/t_database.o: test/database.c lib/database.h lib/commons.h
	$(CC) -o $@ -c test/database.c

$(BUILD)/server.o: server.c lib/communicate.h lib/database.h lib/io.h lib/commons.h
	$(CC) -o $@ -c server.c

$(BUILD)/u_fort.o: lib/fort.c lib/fort.h
	$(CC) -o $@ -c lib/fort.c

$(BUILD)/client.o: client.c lib/communicate.h lib/io.h lib/fort.h
	$(CC) -o $@ -c client.c

.PHONY: clean t_db server client


t_db: $(BIN)/t_db


server: $(BIN)/server


client: $(BIN)/client


clean:
	rm -r $(BUILD)

$(shell mkdir -p $(BUILD))
$(shell mkdir -p $(BIN))
