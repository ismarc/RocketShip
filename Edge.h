/*
** Edge.h
** 
** Made by Matthew Brace
** Login   <mbrace@godwin.lan>
** 
** Started on  Mon Aug 30 22:08:57 2010 Matthew Brace
** Last update Mon Aug 30 22:08:57 2010 Matthew Brace
*/

#ifndef   	EDGE_H_
# define   	EDGE_H_

#include <string>

/**
 * Handles all data associated with an edge leading from a node.
 */
class Edge {
public:
    /**
     * Constructor.
     * @param id The unique name the edge points to.
     * @param label The label to use for the edge.
     */
    Edge(std::string id, std::string label="");
    ~Edge();

    /**
     * @return the label associated with the edge.
     */
    std::string getLabel();
    /**
     * @return the unique name of the node the edge points to.
     */
    std::string getId();
private:
    // Stores the unique name of the node the edge points to.
    std::string _id;
    // Stores the label associated with the edge.
    std::string _label;
};

#endif 	    /* !EDGE_H_ */
