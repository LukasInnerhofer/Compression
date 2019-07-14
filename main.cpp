#include <iostream>
#include <fstream>

#include "compression.h"

int main()
{
	std::ifstream ifs;
	std::ofstream ofs;
	std::vector<char> inData = {};
	char byte;

	ifs = std::ifstream(R"(D:\Users\Lukas Innerhofer\Documents\vs projects\Compression\Release\in)", std::ios_base::binary);
	ofs = std::ofstream(R"(D:\Users\Lukas Innerhofer\Documents\vs projects\Compression\Release\ot)", std::ios_base::binary);

	while (ifs.get(byte))
	{
		inData.push_back(static_cast<uint8_t>(byte));
	}
	//inData = { 'A', 'A', 'A', 'B', 'B', 'C', 'C', 'C', 'C' };
	for (uint8_t outByte : compression::huffman::encode(inData))
	{
		ofs << static_cast<char>(outByte);
	}

	ifs = std::ifstream(R"(D:\Users\Lukas Innerhofer\Documents\vs projects\Compression\Release\ot)", std::ios_base::binary);
	ofs = std::ofstream(R"(D:\Users\Lukas Innerhofer\Documents\vs projects\Compression\Release\_i)", std::ios_base::binary);

	inData.clear();

	while (ifs.get(byte))
	{
		inData.push_back(byte);
	}

	for (uint8_t outByte : compression::huffman::decode(inData))
	{
		ofs << outByte;
	}

	return 0;
}