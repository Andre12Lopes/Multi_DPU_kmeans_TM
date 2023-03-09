#!/bin/bash
echo -e "N_THREADS_DPU\tN_TANSACTIONS\tN_BACHES\tN_DPUS\tCOPY_TIME\tTOTAL_TIME" > results.txt

make clean
# make test NR_BACHES=10 NR_TANSACTIONS=1000 NR_ACCOUNTS=800 NR_DPUS=1

# for (( j = 0; j < 10; j++ )); do
# 	./host/host >> results.txt
# done

for (( i = 1; i < 2560; i *= 2 )); do
	make clean
	make test NR_BACHES=10 NR_TANSACTIONS=1000 NR_ACCOUNTS=1000000 NR_DPUS=$i
	
	for (( j = 0; j < 10; j++ )); do
		./host/host >> results.txt
	done
done

# make clean
# make test NR_BACHES=10 NR_TANSACTIONS=1000 NR_ACCOUNTS=800 NR_DPUS=2554

# for (( j = 0; j < 10; j++ )); do
# 	./host/host >> results.txt
# done