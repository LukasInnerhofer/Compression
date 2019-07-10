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
			static std::vector<bool> tempBitset;

			char byte = 0;
			uint64_t occurences = 0;
			std::pair<node_t *, node_t *> children = { nullptr, nullptr };

			node_t() {}
			node_t(const char byte, const uint64_t &occurences)
			{
				this->byte = byte;
				this->occurences = occurences;
			}
			node_t(node_t *child0, node_t *child1)
			{
				children.first = child0;
				children.second = child1;
			}
			node_t(node_t *child0, node_t *child1, const uint64_t &occurences) : node_t(child0, child1)
			{
				this->occurences = occurences;
			}

			void getCode(std::map<char, std::vector<bool>> &nodes)
			{
				if (children.first != nullptr && children.second != nullptr)
				{
					tempBitset.push_back(true);
					children.first->getCode(nodes);
					tempBitset.erase(tempBitset.end() - 1);
					tempBitset.push_back(false);
					children.second->getCode(nodes);
					tempBitset.erase(tempBitset.end() - 1);
				}
				else
				{
					nodes[byte] = tempBitset;
				}
			}
		};

		std::vector<bool> node_t::tempBitset = {};

		namespace header
		{
			/*
			* \brief		Serialize huffman code into a format which can be written into a file header
			* \param[out]	code Mapping of byte to huffman code
			* \return		Byte string representation of \p code
			*/
			std::vector<char> generate(const std::map<char, std::vector<bool>> &code)
			{
				std::vector<char> header;
				int16_t itBits = 0, itCodeBits = 0;

				for (std::pair<char, std::vector<bool>> pair : code)
				{
					header.push_back(pair.first);
					header.push_back(pair.second.size());
					header.push_back(0);

					for (itCodeBits = pair.second.size() - 1, itBits = 7; itCodeBits >= 0; --itCodeBits, --itBits)
					{
						header[header.size() - 1] |= (pair.second[itCodeBits] << itBits);
						
						if (itBits == 0)
						{
							itBits = 7;
							header.push_back(0);
						}
					}
				}

				header.insert(header.begin(), static_cast<char>(header.size() + 2));
				header.insert(header.begin(), static_cast<char>((header.size() + 1) >> 8));

				return header;
			}
		}

		/**
		* \brief		Huffman encode entire dataset
		*/
		std::vector<char> encode(const std::vector<char> &dataIn)
		{
			std::vector<char> dataOut;
			dataOut.push_back(0);

			std::map<char, uint64_t> byteOccurences = {};
			std::vector<node_t *> rootNodes;
			std::vector<bool> bits;
			node_t node;

			for (char byte : dataIn)
			{
				++byteOccurences[byte];
			}

			for (std::pair<char, uint64_t> pair : byteOccurences)
			{
				rootNodes.push_back(new node_t(pair.first, pair.second));
			}

			while (rootNodes.size() > 1)
			{
				std::sort(rootNodes.begin(), rootNodes.end(), [](const node_t *node0, const node_t *node1) { return node0->occurences < node1->occurences; });

				if (rootNodes.size() >= 2)
				{
					rootNodes.push_back(new node_t(rootNodes[0], rootNodes[1], rootNodes[0]->occurences + rootNodes[1]->occurences));
					rootNodes.erase(rootNodes.begin(), rootNodes.begin() + 2);
				}
			}

			std::map<char, std::vector<bool>> code;
			rootNodes[0]->getCode(code);

			dataOut = header::generate(code);

			int8_t itBits = 7;
			int64_t itCodeBits = 0;
			for (char byte : dataIn)
			{
				itCodeBits = code[byte].size() - 1;
				while (itCodeBits >= 0)
				{
					dataOut[dataOut.size() - 1] |= (code[byte][itCodeBits] << itBits);
					--itCodeBits;
					if (--itBits < 0)
					{
						itBits = 7;
						dataOut.push_back(0);
					}
				}
			}

			// cleanup
			for (node_t *node : rootNodes)
			{
				delete node;
			}

			return dataOut;
		}

		/**
		* \brief		Huffman decode entire dataset
		*/
		std::vector<char> decode(const std::vector<char> &dataIn);
	} // namespace huffman
} // namespace compression