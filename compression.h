/**
* \file		compression.h
* \brief	Interface providing various data compression functions
* \author	Lukas Innerhofer
* \version	1.0
*/

#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <vector>

namespace compression
{
	/**
	* \brief	Run-length encoding
	* \details	Ideal for data containing many longer runs of the same byte.
	*/
	namespace rle
	{
		/**
		* \brief		RL encode entire dataset
		* \remarks		Run-length encoding data that lacks many long byte runs 
		*				may lead to a longer output than input.
		*/
		std::vector<char> encode(const std::vector<char> &dataIn);

		/**
		* \brief		RL decode entire dataset
		*/
		std::vector<char> decode(const std::vector<char> &dataIn);
	}

	namespace huffman
	{
		/**
		* \brief		Huffman encode entire dataset
		*/
		std::vector<char> encode(const std::vector<char> &dataIn);

		/**
		* \brief		Huffman decode entire dataset
		*/
		std::vector<char> decode(const std::vector<char> &dataIn);
	}
}

#endif // COMPRESSION_H