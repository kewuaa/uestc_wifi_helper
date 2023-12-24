OS_BITS=32
TARGET_TRIPLE=x86-windows-gnu

all: encrypt.dll

encrypt.dll: .\lib\out\encrypt.dll
	move $^ $@

.\lib\out:
	mkdir $@

.\lib\out\encrypt.dll: .\lib\encrypt.cpp .\lib\out
	zig c++ -O3 -shared $^ -o $@ -m$(OS_BITS) --target=$(TARGET_TRIPLE)

clean:
	del encrypt.dll
