slime : slimechunkfinder.c
	gcc -o slime -Wall -fexec-charset=GBK -fopt-info -static -Ofast -flto -pipe -fopenmp -march=native -mtune=native -ffast-math -finline-functions -finline-limit=99999999 slimechunkfinder.c

avx512 : slimechunkfinder.c
	gcc -o slime_avx512 -Wall -fexec-charset=GBK -fopt-info -static -Ofast -flto -pipe -fopenmp -mavx512f -mavx512cd -mavx512bw -mavx512dq -mavx512vl -ffast-math -finline-functions -finline-limit=99999999 slimechunkfinder.c

clean :
	rm slime.exe slime_avx512.exe