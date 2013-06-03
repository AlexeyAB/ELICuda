ELICuda
=======

ELICuda - an example for unified using Thrust code on host backends CPU(CPP/OpenMP) and GPU(CUDA)

Divide each byte of the first file, by the corresponding byte from the second file.

Example of use: ELICuda.exe filename1 filename2

ELICuda_nvcc.exe - compiled with nvcc supports OpenMP and CUDA GPGPU
ELICuda_msvc.exe - compiled purely msvc10

To work needed ELICuda_nvcc.exe 2 libraries:
cudart64_50_35.dll
vcomp100.dll

The results of 5000 iterations of the division:
CPU: 3.60 sec
CPU OMP: 1.55 sec
GPU: 0.04 seconds
