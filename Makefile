slime : slimechunkfinder.c
	gcc -o slime -Wall -fexec-charset=GBK -fopt-info -Ofast -flto -pipe -fopenmp -march=native -mtune=native -ffast-math -finline-functions -finline-limit=99999999 slimechunkfinder.c

release : slimechunkfinder.c
	gcc -o slime -Wall -fexec-charset=GBK -s -static -Ofast -flto -pipe -fopenmp -march=native -mtune=native -ffast-math -finline-functions -finline-limit=99999999 slimechunkfinder.c

clean :
	rm slime.exe