all: yatta
yatta:
	cd src && \
	g++ yatta.cpp assembler.cpp cpu.cpp computer.cpp parser.cpp shell.cpp -o ../yatta -Wall -Wextra -Wpedantic -Wformat -std=c++20 && \
	cd ..
clean:
	rm -f yatta