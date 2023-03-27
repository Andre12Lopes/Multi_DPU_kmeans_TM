#!/bin/bash
echo -e "N_THREADS_DPU\tN_DPUS\tN_LOOPS\tN_TANSACTIONS\tCOMM_TIME\tTOTAL_TIME" > results.txt

DPUS="1 500 1000 1500 2000 2500"

make clean
	
for p in $DPUS; do
	make clean
	make test NUM_DPUS=$p
	
	for (( j = 0; j < 1; j++ )); do
		./host/host >> results.txt
	done
done

