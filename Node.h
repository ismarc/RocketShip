/*
** Node.h
** 
** Made by Matthew Brace
** Login   <mbrace@godwin.lan>
** 
** Started on  Fri Aug 27 03:14:25 2010 Matthew Brace
** Last update Fri Aug 27 03:14:25 2010 Matthew Brace
*/

#ifndef   	NODE_H_
# define   	NODE_H_

#include "Edge.h"

#include "llvm/BasicBlock.h"

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <map>

class Node;
typedef boost::shared_ptr<Node> pNode;
typedef std::vector<pNode> Nodes;

/**
 * Handles data, types and operations for all nodes in a graph.
 */
class Node {
public:
    /**
     * Defines the types of nodes, used for determining shapes to
     * draw.
     */
    enum Type {
        START, /** begins a graph */
        ACTIVITY, /** indicates an operation that occurs */
        DECISION, /** indicates a branch in processing */
        END /** indicates the end of a graph */
    };

    /**
     * Constructor.
     * @param identifier The unique integer id to associate with the
     * node.
     * @param type The type of node the object represents.
     */
    Node(int identifier = 0, Type type = ACTIVITY);
    ~Node();

    /**
     * Data retrieval methods
     */
    /**
     * @return the unique id associated with the node.
     */
    int getNodeId();
    /**
     * @return the Node::Type associated with the node.
     */
    Type getNodeType();
    /**
     * @return the std::vector of pointers to Edge objects leading
     * from the node.
     */
    std::vector<Edge*> getNodeEdges();
    /**
     * @return the label assigned to the node.
     */
    std::string getNodeLabel();
    /**
     * @return the name assigned to the node.
     */
    std::string getNodeName();

    /**
     * Data setting methods
     */
    /**
     * Set the node's unique id.
     * @param value The value to set for the id.
     */
    void setNodeId(int value);
    /**
     * Set the node's type.
     * @param value the type to assign to the node.
     */
    void setNodeType(Type value);
    /**
     * Set the node's name
     * @param value the name to assign to the node.
     */
    void setNodeName(std::string value);
    /**
     * Set the node's label
     * @param value the value to assign to the node.
     */
    void setNodeLabel(std::string value);
    void setInstruction(llvm::Instruction* instruction);
    
    /**
     * Add an edge leading from the node.  Each edge from the node
     * must lead to a distinct node (e.g., no duplicates).
     * @param edge The edge to add to the node.
     */
    void addNodeEdge(Edge* edge);
    /**
     * Remove an edge leading from the node.
     * @param edge The edge to remove.
     */
    void removeNodeEdge(Edge* edge);
    /**
     * Retrieve the mappings of labels to BasicBlocks to add
     * associated edges.
     * @return The mapping of labels to BasicBlocks this Node
     * connects to.
     */
    std::map<std::string, llvm::BasicBlock*> getBlockEdges();
private:
    // Stores the associated unique id
    int _nodeId;
    // Stores the node type
    Type _nodeType;
    // Stores the node name
    std::string _nodeName;
    // Stores the node label
    std::string _nodeLabel;
    // Stores each Edge leading from the node.
    std::vector<Edge*> _edges;

    llvm::Instruction* _instruction;
};

#endif 	    /* !NODE_H_ */
