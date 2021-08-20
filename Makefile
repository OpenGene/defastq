DIR_INC = ./inc
DIR_SRC = ./src
DIR_OBJ = ./obj
BINDIR=/usr/local/bin

SRC = $(wildcard ${DIR_SRC}/*.cpp)  
OBJ = $(patsubst %.cpp,${DIR_OBJ}/%.o,$(notdir ${SRC})) 

TARGET = defastq

BIN_TARGET = ${TARGET}

CC = g++
CFLAGS = -std=c++11 -g -o3 -I${DIR_INC}

${BIN_TARGET}:${OBJ}
	$(CC) $(OBJ) ../isa-l/.libs/libisal.a ../libdeflate/libdeflate.a -lpthread -o $@

static:${OBJ}
	$(CC) $(OBJ) -static ../isa-l/.libs/libisal.a ../libdeflate/libdeflate.a -lpthread -o ${BIN_TARGET}
    
${DIR_OBJ}/%.o:${DIR_SRC}/%.cpp make_obj_dir
	$(CC) $(CFLAGS) -O3 -c  $< -o $@

.PHONY:clean
.PHONY:static

clean:
	rm obj/*.o
	rm $(TARGET)

make_obj_dir:
	@if test ! -d $(DIR_OBJ) ; \
	then \
		mkdir $(DIR_OBJ) ; \
	fi

install:
	install $(TARGET) $(BINDIR)/$(TARGET)
	@echo "Installed."
