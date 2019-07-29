#include <iostream>
#include <fstream>

#include <SFML/Graphics.hpp>

#include "compression.h"

int main()
{
	sf::RenderTexture texture;

	std::ifstream ifs;
	std::ofstream ofs;
	std::vector<char> inData = {};
	char byte;

	ifs = std::ifstream(R"(D:\Users\Lukas Innerhofer\Documents\vs projects\Compression\Release\in)", std::ifstream::binary);
	ofs = std::ofstream(R"(D:\Users\Lukas Innerhofer\Documents\vs projects\Compression\Release\ot)", std::ofstream::binary);

	compression::huffman::encode(ifs, ofs);

	/*texture.getTexture().copyToImage().saveToFile("out.bmp");

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
	}*/

	return 0;
}