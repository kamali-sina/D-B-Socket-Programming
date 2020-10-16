CC := gcc
# CCFLAGS += -l SDL2 -l SDL2_image -l SDL2_ttf -l SDL2_mixer

BUILD_DIR=.

all: clean ${BUILD_DIR}/server ${BUILD_DIR}/client

${BUILD_DIR}/server:
	$(CC) server.c -o ${BUILD_DIR}/server

${BUILD_DIR}/client:
	$(CC) client.c -o ${BUILD_DIR}/client

${BUILD_DIR}/rsdl.o: src/rsdl.hpp src/rsdl.cpp
	$(CC) -c src/rsdl.cpp -o ${BUILD_DIR}/rsdl.o

${BUILD_DIR}/Tools.o: Tools.cpp
	$(CC) -c Tools.cpp -o ${BUILD_DIR}/Tools.o

${BUILD_DIR}/Powerup.o: Powerup.cpp
	$(CC) -c Powerup.cpp -o ${BUILD_DIR}/Powerup.o

${BUILD_DIR}/Bullet.o: Bullet.cpp
	$(CC) -c Bullet.cpp -o ${BUILD_DIR}/Bullet.o

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR}/*o ${BUILD_DIR}/*.out ${BUILD_DIR}/server ${BUILD_DIR}/client
run:
	./${BUILD_DIR}/executable.out
