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

#include <SFML/Graphics.hpp>

#include "compression.h"
#include "node.h"

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
		namespace header
		{
			/*
			* \brief		Serialize huffman code into a format which can be written into a file header
			* \param[in]	code Mapping of byte to huffman code
			* \return		Byte string representation of \p code
			*/
			std::vector<char> serialize(const std::map<char, std::vector<bool>> &code)
			{
				std::vector<char> header;
				int16_t itBits = 0;
				uint8_t itCodeBits = 0;

				for (std::pair<char, std::vector<bool>> pair : code)
				{
					header.push_back(pair.first);
					header.push_back(pair.second.size());
					header.push_back(0);

					for (itCodeBits = 0, itBits = 7; itCodeBits < pair.second.size();)
					{
						header[header.size() - 1] |= (pair.second[itCodeBits++] << itBits);
						
						if (--itBits < 0)
						{
							itBits = 7;
							if (itCodeBits < pair.second.size())
							{
								header.push_back(0);
							}
						}
					}
				}

				header.insert(header.begin(), static_cast<char>(header.size() + 2));
				header.insert(header.begin(), static_cast<char>((header.size() + 1) >> 8));

				return header;
			}

			void serialize(const std::map<char, std::vector<bool>> &code, std::ostream &dataOut)
			{
				std::vector<char> header;
				int16_t itBits = 0;
				uint8_t itCodeBits = 0;

				for (std::pair<char, std::vector<bool>> pair : code)
				{
					header.push_back(pair.first);
					header.push_back(pair.second.size());
					header.push_back(0);

					for (itCodeBits = 0, itBits = 7; itCodeBits < pair.second.size();)
					{
						header[header.size() - 1] |= (pair.second[itCodeBits++] << itBits);

						if (--itBits < 0)
						{
							itBits = 7;
							if (itCodeBits < pair.second.size())
							{
								header.push_back(0);
							}
						}
					}
				}

				header.insert(header.begin(), static_cast<char>(header.size() + 2));
				header.insert(header.begin(), static_cast<char>((header.size() + 1) >> 8));

				for (char byte : header)
				{
					dataOut << byte;
				}
			}

			/**
			* \brief		Deserialize header to huffman code
			* \param[in]	header Header to be deserialized
			* \return		Huffman code generated from header
			*/
			std::map<char, std::vector<bool>> deserialize(const std::vector<char> &header)
			{
				std::map<char, std::vector<bool>> code;
				uint8_t byte = 0;
				int16_t codeBitCount = 0;
				int16_t itBits = 7;
				std::vector<char>::const_iterator itHeader = header.begin();
				do
				{
					byte = *itHeader++;
					codeBitCount = *itHeader++;
					itBits = 7;
					while (--codeBitCount >= 0)
					{
						code[byte].push_back(((*itHeader) & (1 << itBits)) >> itBits);

						if (--itBits < 0)
						{
							itBits = 7;
							if (codeBitCount > 0)
							{
								++itHeader;
							}
						}
					}
					++itHeader;
				} while (itHeader != header.end());

				return code;
			}
		}

		/**
		* \brief		Huffman encode entire dataset
		*/
		std::vector<char> encode(const std::vector<char> &dataIn, Node *&topNode)
		{
			std::vector<char> dataOut;
			dataOut.push_back(0);

			std::map<char, uint64_t> byteOccurences = {};
			std::vector<Node *> treeNodes;
			std::vector<bool> bits;
			Node node;

			for (char byte : dataIn)
			{
				++byteOccurences[byte];
			}

			for (std::pair<char, uint64_t> pair : byteOccurences)
			{
				treeNodes.push_back(new Node(pair.first, pair.second));
			}

			while (treeNodes.size() > 1)
			{
				std::sort(treeNodes.begin(), treeNodes.end(), [](const Node *node0, const Node *node1) { return node0->getOccurences() < node1->getOccurences(); });

				if (treeNodes.size() >= 2)
				{
					treeNodes.push_back(new Node(treeNodes[0], treeNodes[1], treeNodes[0]->getOccurences() + treeNodes[1]->getOccurences()));
					treeNodes.erase(treeNodes.begin(), treeNodes.begin() + 2);
				}
			}

			std::map<char, std::vector<bool>> code;
			{
				std::vector<bool> tempBitset;
				treeNodes[0]->getCode(code, tempBitset);
			}
			
			dataOut = header::serialize(code);
			dataOut.insert(dataOut.begin(), static_cast<char>(dataIn.size()));
			dataOut.insert(dataOut.begin(), static_cast<char>(dataIn.size() >> 8));
			dataOut.insert(dataOut.begin(), static_cast<char>(dataIn.size() >> 16));
			dataOut.insert(dataOut.begin(), static_cast<char>(dataIn.size() >> 24));

			dataOut.push_back(0);

			int8_t itBits = 7;
			int64_t itCodeBits = 0;
			for (char byte : dataIn)
			{
				itCodeBits = 0;
				while (itCodeBits < code[byte].size())
				{
					dataOut[dataOut.size() - 1] |= (code[byte][itCodeBits] << itBits);
					++itCodeBits;
					if (--itBits < 0)
					{
						itBits = 7;
						dataOut.push_back(0);
					}
				}
			}

			topNode = treeNodes[0];

			return dataOut;
		}

#ifdef SFML_STATIC
		std::vector<char> encode(const std::vector<char> &dataIn, sf::RenderTexture &texture)
		{
			/* TODO:	Place nodes into matrix. Every node follows following constraints: 
			*				- Be at least 1 to the left/right of your parent
			*				- Be to the left of your 1st child and to the right of your 2nd child
			*			Try to compress the grid by letting every node strive towards its parent.
			*			Draw grid to texture.
			*/
			std::vector<char> dataOut;
			Node *topNode = nullptr;

			dataOut = encode(dataIn, topNode);

			topNode->freeRecursively();
			topNode = nullptr;

			return dataOut;
		}
#endif // SFML_STATIC

		std::vector<char> encode(const std::vector<char> &dataIn)
		{
			Node *topNode = nullptr;
			std::vector<char> dataOut;

			dataOut = encode(dataIn, topNode);

			topNode->freeRecursively();
			topNode = nullptr;

			return dataOut;
		}

		void encode(std::istream &dataIn, std::ostream &dataOut, Node *&topNode)
		{
			std::map<char, uint64_t> byteOccurences = {};
			std::vector<Node *> treeNodes;
			std::vector<bool> bits;
			Node node;

			dataIn.seekg(0, dataIn.beg);

			{
				char byte;
				while (dataIn.get(byte))
				{
					++byteOccurences[byte];
				}
			}

			dataIn.clear();
			
			for (std::pair<char, uint64_t> pair : byteOccurences)
			{
				treeNodes.push_back(new Node(pair.first, pair.second));
			}

			while (treeNodes.size() > 1)
			{
				std::sort(treeNodes.begin(), treeNodes.end(), [](const Node *node0, const Node *node1) { return node0->getOccurences() < node1->getOccurences(); });

				if (treeNodes.size() >= 2)
				{
					treeNodes.push_back(new Node(treeNodes[0], treeNodes[1], treeNodes[0]->getOccurences() + treeNodes[1]->getOccurences()));
					treeNodes.erase(treeNodes.begin(), treeNodes.begin() + 2);
				}
			}

			{
				dataIn.seekg(0, dataIn.end);
				const size_t size = dataIn.tellg();
				dataOut << static_cast<char>(size);
				dataOut << static_cast<char>(size >> 8);
				dataOut << static_cast<char>(size >> 16);
				dataOut << static_cast<char>(size >> 24);
			}

			std::map<char, std::vector<bool>> code;

			{
				std::vector<bool> tempBitset;
				treeNodes[0]->getCode(code, tempBitset);
			}
			
			header::serialize(code, dataOut);

			char outByte = 0, inByte = 0;
			int8_t itBits = 7;
			int64_t itCodeBits = 0;
			dataIn.seekg(0, dataIn.beg);
			while(dataIn.get(inByte))
			{
				itCodeBits = 0;
				while (itCodeBits < code[inByte].size())
				{
					outByte |= (code[inByte][itCodeBits] << itBits);
					++itCodeBits;
					if (--itBits < 0)
					{
						itBits = 7;
						dataOut << outByte;
					}
				}
			}

			topNode = treeNodes[0];
		}

		void encode(std::istream &dataIn, std::ostream &dataOut)
		{
			Node *topNode = nullptr;

			encode(dataIn, dataOut, topNode);

			topNode->freeRecursively();
			topNode = nullptr;
		}

		/**
		* \brief		Huffman decode entire dataset
		*/
		std::vector<char> decode(const std::vector<char> &dataIn)
		{
			std::vector<char> dataOut;

			Node *treeNode = new Node(nullptr, nullptr);
			Node *node = treeNode;

			int16_t itBits = 7;

			uint16_t headerSize = 0;
			uint32_t originalDataSize = 0;

			originalDataSize |= static_cast<uint32_t>(dataIn[3]) & 0x000000FF;
			originalDataSize |= (static_cast<uint32_t>(dataIn[2]) << 8) & 0x0000FF00;
			originalDataSize |= (static_cast<uint32_t>(dataIn[1]) << 16) & 0x00FF0000;
			originalDataSize |= (static_cast<uint32_t>(dataIn[0]) << 24) & 0xFF000000;

			headerSize |= static_cast<uint16_t>(dataIn[5]) & 0x00FF;
			headerSize |= (static_cast<uint16_t>(dataIn[4]) << 8) & 0xFF00;

			std::map<char, std::vector<bool>> code = header::deserialize({ dataIn.begin() + 6, dataIn.begin() + headerSize + 4 });

			for (std::pair<char, std::vector<bool>> pair : code)
			{
				node = treeNode;
				for (std::vector<bool>::iterator itBits = pair.second.begin(); itBits != pair.second.end(); ++itBits)
				{
					if ((*itBits ? node->getChildren().second : node->getChildren().first) == nullptr)
					{
						Node *newNode = new Node(itBits == pair.second.end() - 1 ? pair.first : 0, 0);
						if (*itBits)
						{
							node->setSecondChild(newNode);
						}
						else
						{
							node->setFirstChild(newNode);
						}
					}
					node = (*itBits ? node->getChildren().second : node->getChildren().first);
				}
			}

			std::vector<char>::const_iterator itBytes = dataIn.begin() + headerSize + 4;

			node = treeNode;

			while (true)
			{
				if (node->getChildren().second == nullptr || node->getChildren().first == nullptr)
				{
					dataOut.push_back(node->getByte());
					node = treeNode;
					continue;
				}
				else
				{
					node = ((*itBytes & (1 << itBits)) >> itBits) ? node->getChildren().second : node->getChildren().first;
				}

				if (--itBits < 0)
				{
					itBits = 7;
					if (++itBytes == dataIn.end())
					{
						break;
					}
				}
			}

			treeNode->freeRecursively();
			treeNode = nullptr;

			while (dataOut.size() > originalDataSize)
			{
				dataOut.erase(dataOut.end() - 1);
			}

			return dataOut;
		}
	} // namespace huffman
} // namespace compression