// ELICuda.cpp: определяет точку входа для консольного приложения.
//

// user settings
bool wait_in = true;			// to expect before the exit
const size_t count_it = 5000;	// count of iterations of the loop (division) Repeated division leads to zero results!!!
//#undef __CUDACC__				// or CUDA_ARCH when uses nvcc - compiler. Use CUDA for OpenMP/GPGPU
#define USE_THRUST				// for use Thrust OMP and may be CUDA

#ifdef __CUDACC__
	#define USE_THRUST
	const bool use_cuda = true;
#else
	const bool use_cuda = false;
#endif
//---------------------------------------------------------------------------
		

// std::
#include <iostream>
#include <string>
#include <ctime>
#include <climits>


// thrust::
#ifdef USE_THRUST 
	// For Visual Studio 2010 and CUDA 4.2:
	// In Project Properties -> Configuration Properties -> CUDA C/C++ -> Command Line -> Additional Options: -Xcompiler "/openmp"
	#ifdef _OPENMP
		#define THRUST_HOST_SYSTEM THRUST_HOST_SYSTEM_OMP	// nvcc -Xcompiler "/openmp"
		const bool use_omp = true;
	#else
		const bool use_omp = false;
	#endif
	//#define THRUST_HOST_SYSTEM THRUST_HOST_SYSTEM_TBB

	#include "cuda_runtime.h"
	#include "device_launch_parameters.h"
	#include <thrust/version.h>
	#include <thrust/host_vector.h>
	#include <thrust/device_vector.h>
	#include <thrust/copy.h>
	#include <thrust/transform.h>
	#include <thrust/functional.h>
	
	namespace eli {
		using namespace thrust;
		typedef host_vector<unsigned short int> T_use_h_vec;
#ifdef __CUDACC__
		typedef device_vector<unsigned short int> T_use_d_vec;
#else
		typedef host_vector<unsigned short int> T_use_d_vec;
#endif
		static_assert(sizeof(unsigned short int) == 2, "Must 1 pixel = 2 bytes!");
	};
#else
// std::
	const bool use_omp = false;
	//#include <omp.h>
	#include <vector>
	#include <algorithm>
	#include <functional>
	#define THRUST_MAJOR_VERSION 0
	#define THRUST_MINOR_VERSION 0

	namespace eli {
		using namespace std;
		typedef vector<unsigned short int> T_use_h_vec;
		typedef vector<unsigned short int> T_use_d_vec;
		static_assert(sizeof(unsigned short int) == 2, "Must 1 pixel = 2 bytes!");
	};
#endif


#include "ELIdivision.h"
//---------------------------------------------------------------------------

// task of division images
template<typename T>
bool division(const std::string input_file1, const std::string input_file2, const std::string output_file, const size_t _count_it) {

	try {

		eli::T_ELI_file<T> eli_file1(input_file1);
		std::cout << "Loaded ELI file: " << input_file1 << std::endl;
		eli::T_ELI_file<T> eli_file2(input_file2);
		std::cout << "Loaded ELI file: " << input_file2 << std::endl;

		eli_file1.division(eli_file2);
		std::cout << "Divided" << std::endl;
		if (eli_file1.save_eli_file(output_file)) std::cout << "ELI saved to file: " << output_file << std::endl;
		else std::cout << "Failed save EIL to file: " << output_file << std::endl;

		std::cout << "Process of dividing is performed for the number of iterations equal to: " << _count_it << std::endl;
		std::cout << "in progress..." << std::endl;
		clock_t t1, t2;
		t1 = clock();

		for(int i = 0; i < _count_it; ++i)
			eli_file1.division(eli_file2);

		t2 = clock();
		std::cout << "Time: " << static_cast<float>(t2 - t1)/CLOCKS_PER_SEC << " seconds" << std::endl;



	} catch(...) {
		std::cout << eli::exception_catch("Error catch at: " + THROW_PLACE) << std::endl << std::endl;
		return false;
	}
	return true;
}
//---------------------------------------------------------------------------


int main(int argc, char * argv[])
{
    if(THRUST_MAJOR_VERSION) std::cout << "Thrust v" << THRUST_MAJOR_VERSION << "." << THRUST_MINOR_VERSION << std::endl;

	std::string input_file1, input_file2, output_file;
	//input_file2 = input_file1 = "LENA16.ELI";

	if (argc == 3) {
		input_file1 = argv[1];
		input_file2 = argv[2];

		if(use_omp) std::cout << std::endl << "Use OpenMP";
		std::cout << std::endl << "On CPU:" << std::endl;
		if(division<eli::T_use_h_vec>(input_file1, input_file2, "out_cpu.eli", count_it) ) std::cout << "Done!" << std::endl;
		else std::cout << "Failed!" << std::endl;

		if (use_cuda) {
			std::cout << std::endl << "On GPU:" << std::endl;
			if(division<eli::T_use_d_vec>(input_file1, input_file2, "out_gpu.eli", count_it) ) std::cout << "Done!" << std::endl;
			else std::cout << "Failed!" << std::endl;
		}
	} else {
		std::cout << std::endl << "Arguments not found!" << std::endl << "Use an example: ELICuda.exe filename1 filename2" << std::endl;
	}

	if (wait_in) std::cin >> wait_in;
	return 0;
}

