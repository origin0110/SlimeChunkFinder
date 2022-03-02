slime : slimechunkfinder.c slimechunkfinder.h test.c
	gcc -o slime -Wall -fopt-info -Ofast -flto -pipe -fopenmp -march=native -mtune=native -ffast-math -finline-functions -finline-limit=99999999 slimechunkfinder.c test.c

release : slimechunkfinder.c slimechunkfinder.h test.c
	gcc -o slime -Wall -s -static -Ofast -flto -pipe -fopenmp -march=native -mtune=native -ffast-math -finline-functions -finline-limit=99999999 slimechunkfinder.c test.c

clean :
	rm slime.exe