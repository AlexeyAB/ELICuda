//---------------------------------------------------------------------------
#pragma once
#ifndef ELIdivisionH
#define ELIdivisionH
//---------------------------------------------------------------------------
// Header of ELI-class nothing is known about CUDA/Thrust

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
//---------------------------------------------------------------------------

#ifndef __FUNCTION__
#define __FUNCTION__
#endif

// digit to string
template<typename T>
std::string num2str(T num) {
	std::ostringstream temp;
	temp << num;
	return temp.str();
}

#define THROW_PLACE std::string() + __FILE__ + ":" + num2str(__LINE__) + ": " + __DATE__ + " " + __TIME__ + ", " + __FUNCTION__ +  ") \n-> ("
//---------------------------------------------------------------------------


namespace eli {

// ELI header (512 bytes)
struct T_ELI_header {
	static const size_t header_size = 512;
	union {
		unsigned char raw_array[header_size];
		struct T_par {
			char signature[4];
			unsigned int header_length;
			unsigned int data_offset;
			unsigned int reserved;
			unsigned int image_width;
			unsigned int image_height;
			unsigned int bit_count;
			unsigned int line_length;
			unsigned char reserved_arr[480];

			bool check_header() {
				bool reserved_arr_flag = true;
				for (size_t i = 0; i < sizeof(reserved_arr); ++i) if(reserved_arr[i] != 0) reserved_arr_flag = false;
				return ((strcmp(signature, "ELI") == 0) && (data_offset % 512 == 0) && (reserved == 0) && 
					(image_width*bit_count/8 <= line_length) && reserved_arr_flag);
			}
		} params;
	};
};


// custom exception
struct eli_exception : std::runtime_error {
	eli_exception(std::string msg) : runtime_error(msg) { /* can log to file msg */ }
};

// All about the ELI file
template<typename T_vec>
class T_ELI_file {
	//typedef typename iterator_traits<typename T_vec::iterator>::value_type T_pixel;
	typedef typename T_vec::value_type T_pixel;

	const std::string file_name;
	std::vector<char> vec_of_file;
	size_t file_size;
	T_ELI_header eli_header;
	T_vec vec_image;
	bool flag_zero_pixel;

	// Get array of ELI file for init vector
	static std::vector<char> get_eli_file(std::string const& _file_name)  {
		std::vector<char> vec_src;
		std::ifstream infile(_file_name.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
		if(infile.is_open())
		{
			std::ifstream::pos_type in_file_size = infile.tellg();
			vec_src.resize(in_file_size);
			infile.seekg (0, std::ios::beg);

			// load all data for sequential read (10x faster than HDD random read)
			infile.read(vec_src.data(), in_file_size);	
			infile.close();
		} else {
			throw eli_exception(THROW_PLACE + "Can't open file: " + _file_name);
		}
		return vec_src;	// RVO or move constructor
	}

	// Get array of ELI header for init vector
	static T_ELI_header get_eli_header(std::vector<char> const& vec_src, std::string const& _file_name)  {
		T_ELI_header eli_header;
		copy(vec_src.data(), vec_src.data() + T_ELI_header::header_size, eli_header.raw_array);
		if (!eli_header.params.check_header()) {
			throw eli_exception(THROW_PLACE + "Error in header of file: " + _file_name);
		} 
		return eli_header;	// RVO or move constructor
	}

	// Get array of ELI image for init vector
	static T_vec get_image(std::vector<char> const& _vec_of_file, T_ELI_header const& _eli_header)  {
		const size_t size_of_image = _eli_header.params.image_height * _eli_header.params.image_width;
		std::vector<T_pixel> h_vec(size_of_image);
		const size_t max_size_of_file = 
			_eli_header.params.data_offset + _eli_header.params.line_length*(_eli_header.params.image_height-1) + sizeof(T_pixel)*(_eli_header.params.image_width-1);
		// check it once and not need to check for each iteration in loop by .at()
		if (max_size_of_file > _vec_of_file.size()) throw std::out_of_range(THROW_PLACE + "out of range in _vec_of_file.");	

		for(size_t i = 0; i < _eli_header.params.image_height; ++i) {
			for(size_t k = 0; k < _eli_header.params.image_width; ++k) {
				const T_pixel val = 
					*(reinterpret_cast<const T_pixel*>(_vec_of_file.data() + _eli_header.params.data_offset + _eli_header.params.line_length*i + sizeof(T_pixel)*k));
				h_vec[i*_eli_header.params.image_width + k] = val;
			}
		}
		return h_vec;	// RVO or move constructor, or host -> device copy
	}

public:
	T_ELI_file(std::string _file_name) 
		: file_name(_file_name), vec_of_file(get_eli_file(_file_name) ), eli_header(get_eli_header(vec_of_file, _file_name) ),
		vec_image(get_image(vec_of_file, eli_header) ), file_size(vec_of_file.size())
	{ 
		//std::vector<char>().swap(vec_of_file);	// benefit: reduce the uses of memory, or reserve memory for next allocation

		// find pixel equal to zero
		if(find(vec_image.begin(), vec_image.end(), 0) != vec_image.end()) flag_zero_pixel = true;
		else flag_zero_pixel = false;		
	}

	// compare image size of two ELI files
	bool compare_size(T_ELI_file<T_vec> const& eli_file) const {
		return (vec_image.size() == eli_file.vec_image.size() && 
			eli_header.params.image_height == eli_file.eli_header.params.image_height && 
			eli_header.params.image_width == eli_file.eli_header.params.image_width && 
			eli_header.params.bit_count == eli_file.eli_header.params.bit_count);
	}

	// divide each pixel of two ELI images
	void division(T_ELI_file<T_vec> const& eli_file) {
		if(!compare_size(eli_file)) throw eli_exception(THROW_PLACE + "ELI Files are not equal!: " + file_name + " and: " + eli_file.file_name);
		if(eli_file.flag_zero_pixel) throw eli_exception(THROW_PLACE + "ELI image contain zero pixel! Can't divide by zero: " + file_name + " and: " + eli_file.file_name);

		transform(vec_image.begin(), vec_image.end(), 
			eli_file.vec_image.begin(), vec_image.begin(), divides<T_pixel>());
	}

	// save ELI to file
	bool save_eli_file(std::string const _file_name)  {
		vec_of_file.resize(file_size);
		copy(eli_header.raw_array, eli_header.raw_array + T_ELI_header::header_size, vec_of_file.data());

		std::vector<T_pixel> _vec_image(vec_image.size());
		copy(vec_image.begin(), vec_image.end(), _vec_image.data());

		for(size_t i = 0; i < eli_header.params.image_height; ++i) {
			for(size_t k = 0; k < eli_header.params.image_width; ++k) {
				const T_pixel val = _vec_image[i*eli_header.params.image_width + k];
				*(reinterpret_cast<T_pixel*>(vec_of_file.data() + eli_header.params.data_offset + eli_header.params.line_length*i + sizeof(T_pixel)*k)) = val;
			}
		}

		std::ofstream outfile(_file_name, std::ios::out | std::ios::binary | std::ios::trunc);
		if(outfile.is_open()) {
			std::ofstream::pos_type out_file_size = vec_of_file.size();
			outfile.write(vec_of_file.data(), out_file_size);
			outfile.close();
		} else {
			throw eli_exception(THROW_PLACE + "Can't open file: " + _file_name);
		}

		return true;
	}

};
//---------------------------------------------------------------------------

// catch std exception and return it as std::string
std::string exception_catch(std::string throw_macros = "")
{
		std::string excmsg = "";
		try
		{
			throw;
		}
		catch (eli_exception& e)
		{
			excmsg = std::string("Exception eli_exception: ") + e.what();
		}
		catch (std::out_of_range& e)
		{
			excmsg = std::string("Exception out_of_range: ") + e.what();
		}
		catch (std::runtime_error& e)
		{
			excmsg = std::string("Exception std::runtime_error: ") + e.what();
		}
		catch (std::exception& e)
		{
			excmsg = std::string("Exception std::exception: ") + e.what();
		}
		catch(...)
		{
			excmsg = "Unknown error";
		}
		excmsg = "(" + throw_macros + excmsg + ")";
		return excmsg;

}

};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#endif	// ELIdivisionH