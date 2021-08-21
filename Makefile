CC=mpicc
MAIN=projectmain
OUT=progetto
EXT=.o
PROCESSES=1

main.o: clean $(MAIN)$(EXT)
	$(CC) $(MAIN)$(EXT) -o $(OUT)$(EXT)

$(MAIN)$(EXT):
	$(CC) -c $(MAIN).c

clean:
	rm -f *.o

run:
	mpirun --allow-run-as-root -np $(PROCESSES) $(OUT)$(EXT) --oversubscribe
