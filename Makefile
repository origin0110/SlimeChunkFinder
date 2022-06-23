slime : slimechunkfinder.c slimechunk.c slimechunk.h
	gcc -o slime -Wall -fexec-charset=GBK -fopt-info -static -Ofast -flto -pipe -fopenmp -march=native -mtune=native -ffast-math -finline-functions -finline-limit=99999999 slimechunkfinder.c slimechunk.c

clean :
	rm slime.exe