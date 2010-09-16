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
#include <map>

class Block;
typedef boost::shared_ptr<Block> pBlock;

/**
 * Represents a block within the file.  A block is made up of a series of
 * instructions.  This is represented internally as a series of Node
 * objects.
 */
class Block {
public:
    /**
     * Constructor, creates the Block object with the supplied
     * identifier and label.  The identifier is expected to be
     * globally unique, but no verification is performed by the
     * object.
     * @param identifier The unique reference for this Block
     * @param label The (optional) label to associate with this Block
     */
    Block(unsigned int identifier, std::string label="");
    /**
     * Destructor, does not explicitely free any resources, by may
     * cause any Node objects to go out of scope (via shared pointers)
     * and be deallocated.
     */
    ~Block();

    /**
     * @return The unique identifier for this Block
     */
    unsigned int getId();
    /**
     * @return The label associated with this Block
     */
    std::string getLabel();
    /**
     * @return the ordered list of Nodes representing instructions.
     */
    Nodes getNodes();

    /**
     * @param value Value to set the unique identifier to
     */
    void setId(unsigned int value);
    /**
     * @param value Value to set the associated label to
     */
    void setLabel(std::string value);
    /**
     * Appends a Node object to the list of instructions associated
     * with this Block.
     * @param node The node to append to the list.
     */
    void appendNode(pNode node);

    int findEdge(llvm::BasicBlock* block, std::map<llvm::BasicBlock*, pBlock> blocks);
    void processNodes(std::map<llvm::BasicBlock*, pBlock> blocks);
private:
    /**
     * The unique identifier for this Block.
     */
    unsigned int _id;
    /**
     * The label associated with this Block.
     */
    std::string _label;
    /**
     * The list of Nodes representing instructions for this Block.
     */
    Nodes _nodes;
}; // end Block

#endif /* !BLOCK_H_ */
