#include "node.h"

namespace compression
{
	namespace huffman
	{
		void Node::freeRecursively()
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

		void Node::getCode(std::map<char, std::vector<bool>> &code, std::vector<bool> &tempBitset)
		{
			if (children.first != nullptr && children.second != nullptr)
			{
				tempBitset.push_back(false);
				children.first->getCode(code, tempBitset);
				tempBitset.erase(tempBitset.end() - 1);
				tempBitset.push_back(true);
				children.second->getCode(code, tempBitset);
				tempBitset.erase(tempBitset.end() - 1);
			}
			else
			{
				code[byte] = tempBitset;
			}
		}
	}
}