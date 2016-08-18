mpirun -rr -np 2  -bootstrap ssh PONCOS /work/applications/lama.install/share/examples/solver/cg_solver.exe /work/applications/lama.install/data/2D5P_2000 LOG_NO T8
mpirun -rr -np 16 -bootstrap ssh PONCOS /work/applications/mpifast.install/bin/mpiblast -d drosoph.nt -i /work/applications/mpifast.install/query/melano500m.20059.fa -p blastn -o /tmp/result.txt
mpirun -rr -np 2  -bootstrap ssh PONCOS /work/applications/lama.install/share/examples/solver/cg_solver.exe /work/applications/lama.install/data/2D5P_2000 LOG_NO T8
mpirun -rr -np 16 -bootstrap ssh PONCOS /work/applications/mpifast.install/bin/mpiblast -d drosoph.nt -i /work/applications/mpifast.install/query/melano500m.20059.fa -p blastn -o /tmp/result.txt
