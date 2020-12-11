all: diskinfo disklist diskget diskput

diskinfo:diskinfo.c
	gcc -Wall -o diskinfo diskinfo.c -std=c99

disklist:disklist.c
	gcc -Wall -o disklist disklist.c -std=c99

diskget:diskget.c
	gcc -Wall -o diskget diskget.c  -std=c99
diskput:diskput.c
	gcc -Wall -o diskput diskput.c  -std=c99