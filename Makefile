CC=mpicc
MAIN=word_count_pack
OUT=progetto
EXT=.o
PROCESSES=2

main.o: clean $(MAIN)$(EXT)
	$(CC) $(MAIN)$(EXT) -o $(OUT)$(EXT)

$(MAIN)$(EXT):
	$(CC) -c $(MAIN).c

clean:
	rm -f *.o

run:
	mpirun --mca btl_vader_single_copy_mechanism none --allow-run-as-root -np $(PROCESSES) $(OUT)$(EXT) --oversubscribe
