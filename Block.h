/*
** Block.h
**
** Made by Matthew Brace
**
** Created on Mon Sep 13 20:03:12 2010 Matthew Brace
*/

#ifndef  BLOCK_H_
# define BLOCK_H_

#include "Node.h"

#include "llvm/BasicBlock.h"

#include <boost/shared_ptr.hpp>
#include <vector>

class Block {
public:
    Block(unsigned int identifier, llvm::BasicBlock* block, std::string label="");
    ~Block();

    unsigned int getId();
    std::string getLabel();
    Nodes getNodes();
    
    void setId(unsigned int value);
    void setLabel(std::string value);
    void appendNode(pNode node);
private:
    unsigned int _id;
    std::string _label;
    Nodes _nodes;
}; // end Block

#endif /* !BLOCK_H_ */
