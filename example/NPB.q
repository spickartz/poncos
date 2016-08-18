mpirun -genv OMP_NUM_THREADS=8 -rr -bootstrap ssh -np 2  PONCOS /work/applications/NPB3.3.1-MZ/NPB3.3-MZ-MPI/bin/lu-mz.D.2
mpirun -genv OMP_NUM_THREADS=8 -rr -bootstrap ssh -np 2  PONCOS /work/applications/NPB3.3.1-MZ/NPB3.3-MZ-MPI/bin/bt-mz.D.2
mpirun -genv OMP_NUM_THREADS=8 -rr -bootstrap ssh -np 2  PONCOS /work/applications/NPB3.3.1-MZ/NPB3.3-MZ-MPI/bin/sp-mz.D.2
mpirun                         -rr -bootstrap ssh -np 16 PONCOS /work/applications/NPB3.3.1/NPB3.3-MPI/bin/bt.D.16
mpirun                         -rr -bootstrap ssh -np 16 PONCOS /work/applications/NPB3.3.1/NPB3.3-MPI/bin/cg.D.16
mpirun                         -rr -bootstrap ssh -np 16 PONCOS /work/applications/NPB3.3.1/NPB3.3-MPI/bin/ep.D.16
mpirun                         -rr -bootstrap ssh -np 16 PONCOS /work/applications/NPB3.3.1/NPB3.3-MPI/bin/ft.D.16
mpirun                         -rr -bootstrap ssh -np 16 PONCOS /work/applications/NPB3.3.1/NPB3.3-MPI/bin/lu.D.16
mpirun                         -rr -bootstrap ssh -np 16 PONCOS /work/applications/NPB3.3.1/NPB3.3-MPI/bin/mg.D.16
mpirun                         -rr -bootstrap ssh -np 16 PONCOS /work/applications/NPB3.3.1/NPB3.3-MPI/bin/sp.D.16
