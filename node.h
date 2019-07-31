/**
* \file		node.h
* \brief	Provides node class specifically for huffman trees
* \author	Lukas Innerhofer
* \version	1.0
*/

#include <map>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Transformable.hpp>

#ifndef NODE_H
#define NODE_H

namespace compression
{
	namespace huffman
	{
		class Node
		{
		protected:
			char byte = 0;
			uint64_t occurences = 0;
			std::pair<Node *, Node *> children = { nullptr, nullptr };

		public:
			inline char getByte() const { return byte; }
			inline uint64_t getOccurences() const { return occurences; }
			inline std::pair<Node *, Node *> getChildren() const { return children; }

			inline void setChildren(const std::pair<Node *, Node *> &children) { this->children = children; }
			inline void setFirstChild(Node *child) { children.first = child; }
			inline void setSecondChild(Node *child) { children.second = child; }

			inline Node() {}
			inline Node(const char byte, const uint64_t &occurences)
			{
				this->byte = byte;
				this->occurences = occurences;
			}
			inline Node(Node *child0, Node *child1)
			{
				children.first = child0;
				children.second = child1;
			}
			inline Node(Node *child0, Node *child1, const uint64_t &occurences) : Node(child0, child1)
			{
				this->occurences = occurences;
			}

			void freeRecursively();

			void getCode(std::map<char, std::vector<bool>> &code, std::vector<bool> &tempBitset);
		};
	}
}

namespace sf
{
	class Node : public compression::huffman::Node, public sf::Drawable
	{
	private:
		sf::CircleShape circle;

	protected:
		virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const;

	public:
		inline float getRadius() const { return circle.getRadius(); }
		inline Vector2f getPosition() const { return circle.getPosition(); }

		inline void setRadius(const float &radius) { circle.setRadius(radius); }
		inline void setPosition(const sf::Vector2f &position) { circle.setPosition(position); }

		Node(compression::huffman::Node node) : compression::huffman::Node(node), sf::Drawable() { circle = sf::CircleShape(10); }
	};
}

#endif // NODE_H