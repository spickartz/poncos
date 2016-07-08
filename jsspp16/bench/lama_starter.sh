#!/bin/sh +x

source /etc/profile.d/modules.sh
module load gcc/4.9
module load binutils/2.25
module load mkl
module load mpi.intel
module load ccomp/intel


#export KMP_AFFINITY="explicit,proclist=[0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15,16,24,17,25,18,26,19,27,20,28,21,29,22,30,23,31],verbose,granularity=fine"
export OMP_NUM_THREADS=${1}

echo $KMP_AFFINITY
echo $OMP_NUM_THREADS

rm output/time-lama-"${1}".txt
touch output/time-lama-"${1}".txt

export LD_LIBRARY_PATH=/work/applications/boost_1_58_0_intel/stage/lib:/work/applications/lama.install/lib:$LD_LIBRARY_PATH


#while true; do
    start="`date +%s.%N`"
    mpirun -np 1 -genv I_MPI_FABRICS shm -bootstrap ssh /work/applications/lama.install/share/examples/solver/cg_solver.exe /work/applications/lama.install/data/2D5P_2000 LOG_NO T"${1}"
    stop="`date +%s.%N`"
    echo "${stop} - ${start}" | bc -l >> "output/time-lama-${1}".txt
#done
