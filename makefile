src := $(wildcard src/*.c)
out := $(src:src/%.c=out/%.o)

inc_dir := include/

bin := pt.exe
def := DEBUG
opt := -Wall -std=c11 -c -O2 -msse2
lib :=

out/%.o: src/%.c
	gcc $(opt) $(def:%=-D%) $< -o $@ -I$(inc_dir)
$(bin): $(out)
	gcc $^ -o $@ -pthread $(lib:%=-l%)
clean:
	rm out/*
	rm $(bin)