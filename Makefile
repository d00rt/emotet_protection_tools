CC = i686-w64-mingw32-gcc
CFLAGS= -Wall
LIBS = -lshlwapi
OUTPUT_DIR = bin
SOURCES_DIR = src
HEADERS = $(SOURCES_DIR)/utils.h

DETECTET = $(OUTPUT_DIR)/Detectet.exe
DETECTET_SOURCES = $(SOURCES_DIR)/detectet.c

PROTECTET = $(OUTPUT_DIR)/Protectet.exe
PROTECTET_SOURCES = $(SOURCES_DIR)/protectet.c
PROTECTET_CFLAGS = -mwindows

all: output_dir $(DETECTET) $(PROTECTET)

.PHONY: clean $(DETECTET) $(PROTECTET) output_dir
$(DETECTET):
	$(CC) $(CFLAGS) -o $(DETECTET) $(DETECTET_SOURCES) $(HEADERS) $(LIBS)

$(PROTECTET):
	$(CC) $(CFLAGS) $(PROTECTET_CFLAGS) -o $(PROTECTET) $(PROTECTET_SOURCES) $(HEADERS) $(LIBS)

output_dir:
	mkdir -p $(OUTPUT_DIR)

clean:
	rm -rf $(OUTPUT_DIR)

