/**
* \file		compression.cpp
* \brief	Implements compression functions
* \author	Lukas Innerhofer
* \version	1.0
*/

#include <algorithm>
#include <bitset>
#include <map>
#include <iostream>

#include "compression.h"

namespace compression
{
	/**
	* \brief	Run-length encoding
	*/
	namespace rle
	{
		/**
		* \details		Iterates entire dataset identifying byte runs and encoding them 
		*				as pairs of { size_of_byterun, byte } into the output data.
		* \param[in]	dataIn	Data to be encoded
		* \return		RL encoded data
		*/
		std::vector<char> encode(const std::vector<char> &dataIn)
		{
			std::vector<char> dataOut = {};
			uint64_t byteRunCount = 0;
			char startByte = 0;

			for (std::vector<char>::const_iterator itBytes = dataIn.begin(); itBytes != dataIn.end(); ++itBytes, byteRunCount = 0)
			{
				// Determine, how often the current byte occurs in succession
				startByte = *itBytes;
				do
				{
					++byteRunCount;
				} while (itBytes + byteRunCount != dataIn.end() && *(itBytes + byteRunCount) == startByte);

				dataOut.push_back(byteRunCount);
				dataOut.push_back(startByte);

				itBytes += byteRunCount - 1;
			}

			return dataOut;
		}

		/**
		* \details		Iterates entire dataset extracting {size_of_byterun, byte }
		*				and writes the byterun to output
		* \param[in]	dataIn	Data to be decoded
		* \return		RL decoded data
		*/
		std::vector<char> decode(const std::vector<char> &dataIn)
		{
			std::vector<char> dataOut = {};
			uint64_t byteRunCount = 0;
			char byte = 0;

			for (std::vector<char>::const_iterator itBytes = dataIn.begin(); itBytes != dataIn.end(); itBytes += 2, byteRunCount = 0)
			{
				byteRunCount = *itBytes;
				byte = *(itBytes + 1);

				for (uint64_t i = 0; i < byteRunCount; ++i)
				{
					dataOut.push_back(byte);
				}
			}

			return dataOut;
		}
	} // namespace rle

	namespace huffman
	{
		struct node_t
		{
			char byte = 0;
			uint64_t occurences = 0;
			std::pair<std::shared_ptr<node_t>, std::shared_ptr<node_t>> children = { nullptr, nullptr };

			node_t() {}
			node_t(const char byte, const uint64_t &occurences)
			{
				this->byte = byte;
				this->occurences = occurences;
			}
			node_t(const std::shared_ptr<node_t> child0, const std::shared_ptr<node_t> child1)
			{
				children.first = child0;
				children.second = child1;
			}
		};

		/**
		* \brief		Huffman encode entire dataset
		*/
		std::vector<char> encode(const std::vector<char> &dataIn)
		{
			std::map<char, uint64_t> byteOccurences = {};
			std::vector<std::shared_ptr<node_t>> rootNodes;
			std::vector<bool> bits;

			for (char byte : dataIn)
			{
				++byteOccurences[byte];
			}

			for (std::pair<char, uint64_t> pair : byteOccurences)
			{
				rootNodes.push_back(std::shared_ptr<node_t>(new node_t(pair.first, pair.second)));
			}

			while (rootNodes.size() > 1)
			{
				std::sort(rootNodes.begin(), rootNodes.end(), [](const std::shared_ptr<node_t> node0, const std::shared_ptr<node_t> node1) { return node0->occurences < node1->occurences; });

				if (rootNodes.size() >= 2)
				{
					rootNodes.push_back(std::shared_ptr<node_t>(new node_t(rootNodes[0], rootNodes[1])));
					rootNodes.erase(rootNodes.begin(), rootNodes.begin() + 2);
				}
			}



			return std::vector<char>();
		}

		/**
		* \brief		Huffman decode entire dataset
		*/
		std::vector<char> decode(const std::vector<char> &dataIn);
	} // namespace huffman
} // namespace compression