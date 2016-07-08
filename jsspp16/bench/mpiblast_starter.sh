#!/bin/sh

rm output/time-blast-"${1}".txt
touch output/time-blast-"${1}".txt

source /etc/profile.d/modules.sh
module purge
module load lrz/default
module unload gcc
module unload ccomp/intel
module unload mpi.intel

module load gcc/4.8
module load ccomp/intel/14.0
module load mpi.intel/4.1


export MPIBLAST_SHARED=/work/applications/mpifast.install/SHARED

#while true; do
    start="`date +%s.%N`"
    mpirun -genv I_MPI_FABRICS shm -bootstrap ssh -n "${1}" /work/applications/mpifast.install/bin/mpiblast -d drosoph.nt -i /work/applications/mpifast.install/query/melano500m.20059.fa -p blastn -o /tmp/result.txt
    stop="`date +%s.%N`"
    echo "${stop} - ${start}" | bc -l >> "/work/co/output/time-blast-${1}".txt
#done
