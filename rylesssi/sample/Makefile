all: ssi inf args

ssi: ssi.c
	gcc ssi.c -lreadline -lhistory -ltermcap -o ssi

inf: inf.c
	gcc inf.c -o inf

args: args.c
	gcc args.c -o args

clean:
	rm -f *.o
	rm -f ssi
	rm -f inf
	rm -f args
