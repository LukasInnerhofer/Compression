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

			void freeRecursively()
			{
				if (children.first != nullptr)
				{
					children.first->freeRecursively();
					children.first = nullptr;
				}
				if (children.second != nullptr)
				{
					 
					children.second->freeRecursively();
					children.second = nullptr;
				}
				delete this;
			}

			void getCode(std::map<char, std::vector<bool>> &nodes)
			{
				if (children.first != nullptr && children.second != nullptr)
				{
					tempBitset.push_back(false);
					children.first->getCode(nodes);
					tempBitset.erase(tempBitset.end() - 1);
					tempBitset.push_back(true);
					children.second->getCode(nodes);
					tempBitset.erase(tempBitset.end() - 1);
				}
				else
				{
					nodes[byte] = tempBitset;
				}
			}

			void getDepth(uint64_t &totalDepth, uint64_t currentDepth = 0)
			{
				if (children.first != nullptr)
				{
					++currentDepth;
					children.first->getDepth(totalDepth, currentDepth);
					--currentDepth;
				}
				if (children.second != nullptr)
				{
					++currentDepth;
					children.second->getDepth(totalDepth, currentDepth);
					--currentDepth;
				}
				if (children.first == nullptr && children.second == nullptr)
				{
					if (currentDepth > totalDepth)
					{
						totalDepth = currentDepth;
					}
				}
			}

			void drawRecursively(sf::RenderTarget &target, sf::CircleShape &shape, uint64_t itDepth, const uint64_t &totalDepth, const uint8_t &minSpace)
			{
				target.draw(shape);

				const float deltaX = (std::pow(2, totalDepth - itDepth - 1) / 2.0f) * (minSpace + shape.getRadius() * 2), deltaY = shape.getRadius() * 2;

				if (children.first != nullptr)
				{
					++itDepth;
					shape.setPosition(shape.getPosition().x - deltaX, shape.getPosition().y + deltaY);
					children.first->drawRecursively(target, shape, itDepth, totalDepth, minSpace);
					--itDepth;
					shape.setPosition(shape.getPosition().x + deltaX, shape.getPosition().y - deltaY);
				}
				if (children.second != nullptr)
				{
					++itDepth;
					shape.setPosition(shape.getPosition().x + deltaX, shape.getPosition().y + deltaY);
					children.second->drawRecursively(target, shape, itDepth, totalDepth, minSpace);
					--itDepth;
					shape.setPosition(shape.getPosition().x - deltaX, shape.getPosition().y - deltaY);
				}
			}
		};

		std::vector<bool> node_t::tempBitset = {};

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
		std::vector<char> encode(const std::vector<char> &dataIn, node_t *&topNode)
		{
			std::vector<char> dataOut;
			dataOut.push_back(0);

			std::map<char, uint64_t> byteOccurences = {};
			std::vector<node_t *> treeNodes;
			std::vector<bool> bits;
			node_t node;

			for (char byte : dataIn)
			{
				++byteOccurences[byte];
			}

			for (std::pair<char, uint64_t> pair : byteOccurences)
			{
				treeNodes.push_back(new node_t(pair.first, pair.second));
			}

			while (treeNodes.size() > 1)
			{
				std::sort(treeNodes.begin(), treeNodes.end(), [](const node_t *node0, const node_t *node1) { return node0->occurences < node1->occurences; });

				if (treeNodes.size() >= 2)
				{
					treeNodes.push_back(new node_t(treeNodes[0], treeNodes[1], treeNodes[0]->occurences + treeNodes[1]->occurences));
					treeNodes.erase(treeNodes.begin(), treeNodes.begin() + 2);
				}
			}

			std::map<char, std::vector<bool>> code;
			treeNodes[0]->getCode(code);

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
			node_t *topNode = nullptr;
			std::vector<char> dataOut;
			uint64_t depth = 0;
			constexpr float radius = 1;
			constexpr uint8_t minSpace = 1;
			sf::CircleShape circle(radius);
			uint64_t itDepth = 0;

			dataOut = encode(dataIn, topNode);

			topNode->getDepth(depth);

			texture.create(std::pow(2, depth) * (2 * radius + minSpace) - minSpace, 2 * radius * (depth + 1));

			circle.setPosition(texture.getSize().x / 2, 0);

			topNode->drawRecursively(texture, circle, itDepth, depth, minSpace);

			topNode->freeRecursively();
			topNode = nullptr;

			return dataOut;
		}
#endif // SFML_STATIC

		std::vector<char> encode(const std::vector<char> &dataIn)
		{
			node_t *topNode = nullptr;
			std::vector<char> dataOut;

			dataOut = encode(dataIn, topNode);

			topNode->freeRecursively();
			topNode = nullptr;

			return dataOut;
		}

		void encode(std::istream &dataIn, std::ostream &dataOut, node_t *&topNode)
		{
			std::map<char, uint64_t> byteOccurences = {};
			std::vector<node_t *> treeNodes;
			std::vector<bool> bits;
			node_t node;

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
				treeNodes.push_back(new node_t(pair.first, pair.second));
			}

			while (treeNodes.size() > 1)
			{
				std::sort(treeNodes.begin(), treeNodes.end(), [](const node_t *node0, const node_t *node1) { return node0->occurences < node1->occurences; });

				if (treeNodes.size() >= 2)
				{
					treeNodes.push_back(new node_t(treeNodes[0], treeNodes[1], treeNodes[0]->occurences + treeNodes[1]->occurences));
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
			treeNodes[0]->getCode(code);
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
			node_t *topNode = nullptr;

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

			node_t *treeNode = new node_t(nullptr, nullptr);
			node_t *node = treeNode;

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
					if ((*itBits ? node->children.second : node->children.first) == nullptr)
					{
						(*itBits ? node->children.second : node->children.first) = new node_t(itBits == pair.second.end() - 1 ? pair.first : 0, 0);
					}
					node = (*itBits ? node->children.second : node->children.first);
				}
			}

			std::vector<char>::const_iterator itBytes = dataIn.begin() + headerSize + 4;

			node = treeNode;

			while (true)
			{
				if (node->children.second == nullptr || node->children.first == nullptr)
				{
					dataOut.push_back(node->byte);
					node = treeNode;
					continue;
				}
				else
				{
					node = ((*itBytes & (1 << itBits)) >> itBits) ? node->children.second : node->children.first;
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