/*****************************************************************************
 Program file name: graph.h					OS: Windows 7  Assignment #: 2
 Programmer: Steven Williams			 Class: CPTR143	   Date: 12 April 2010
 Compiler: GNU GCC compiler

 Assistance/references: Nyhoff: ADTs (etc) chapter 16
  cplusplus.com - vector reference
  http://stackoverflow.com/questions/2292202/while-loop-with-try-catch-fails-
  at-bad-cin-input		self-explanatory
 Description: This program builds an undirected graph and uses breadth-first
  and depth-first search algorithms to determine whether the graph is
  connected.
 Inputs: Vertex labels, edge endpoints
 Outputs: List of vertices and their names, whether graph is connected
 Special Comments: I templatized this so I can use it for other data types in
  the future.
~~~~~~~~~~~~~~~~~~~~~~~~~~Grading Criteria~~~~~~~~~~~~~~~~~~~~~~~~~~
 Assignment Requirements: ___/3.0
 Code Format/Cosmetics: ___/3.0
 Header & Code Comments: ___/2.0
 Output Format/Cosmetics: ___/2.0

 ***Does Not Compile***: ___ (-10.0)
 ***Late Work Deduction***: ___ (-0.5/day)
 Total Grade: ___/10.0

*****************************************************************************/

#include <algorithm>
#include <iostream>
#include <vector>
#include "queue.h"
#include "stack.h"

template <typename elementType>
void validInput( elementType &store, string errorMessage )		// function to get valid input from the user for any data type
{
	while( !(cin >> store) )
	{
		cin.clear();
		cin.ignore(std::numeric_limits<streamsize>::max(),'\n');
		cout << errorMessage;
	}
}

template <typename elementType>
class Graph
{
	private:
		class EdgeArray
		{
			public:
				int vertex[3];		// links two vertices (data[0] is unused)
				EdgeArray *link[3];	// points to next two vertex links
		};
		class VertexInfo
		{
			public:
				elementType data;	// vertex name (or data)
				EdgeArray *first;	// points to first edge
		};
		vector<VertexInfo> vertexList;
	public:
		Graph();					// default constructor
		elementType data(int);
		// void build(void);
		bool connect(int,int);		// connects node a to node b
		// bool dfs(void);						// uses dfs to check tree connectivity
		// void dfs(int,vector<bool>&);		// depth-first search
		// bool bfs(void);						// uses bfs to check tree connectivity
		// void bfs(int,vector<bool>&);		// breadth-first search
		void display(void);			// print the adjacency list and connected nodes
};
template <typename elementType>
inline Graph<elementType>::Graph()
{ 
	VertexInfo v;
	vertexList.push_back(v);		// put filler in: vertexList[1] is first vertex according to convention
}
template <typename elementType>

template <typename elementType>
bool Graph<elementType>::connect(int one, int two)
{
	if( one >= vertexList.size() || two >= vertexList.size() || one < 0 || two < 0 )
		return false;
	EdgeArray *newEdge = new EdgeArray;
	EdgeArray *ptr = vertexList[one].first, *ptr2 = vertexList[two].first, *ptrTrail, *ptr2Trail;
	ptrTrail = ptr;
	ptr2Trail = ptr2;
	while( ptr != NULL )	// march ptr to end of list of vertices out of [one]
	{
		if( ptr->vertex[1] == one )
			ptr = ptr->link[1];
		else
			ptr = ptr->link[2];
		ptrTrail = ptr;
	}
	while( ptr2 != NULL )	// march ptr2 to end of list of vertices out of [two]
	{
		if( ptr2->vertex[1] == two )
			ptr2 = ptr2->link[1];
		else
			ptr2 = ptr2->link[2];
		ptr2Trail = ptr2;
	}
	// now ptr and ptr2 are ready to point to a new edge
	newEdge->vertex[1] = one;
	newEdge->vertex[2] = two;
	newEdge->link[1] = ptr;
	newEdge->link[2] = ptr2;
	if( ptrTrail->vertex[1] == one )
		ptrTrail->link[1] = newEdge;
	else
		ptrTrail->link[2] = newEdge;
	if( ptrTrail->vertex[1] == two )
		ptr2Trail->link[1] = newEdge;
	else
		ptr2Trail->link[2] = newEdge;
	
	
	
	
	
}
	vector<bool> visited( vertexList.size(), false );
	dfs( 1, visited );
	for( int i = 1; i < visited.size(); i++ )
	{
		if( visited[i] == 0 )
			return false;
	}
	return true;
}
template <typename elementType>
void Graph<elementType>::display(void)
{
	EdgeArray *ptr;
	int startEnd = 1, otherEnd = 2;
	for( int i = 1; i < vertexList.size(); i++ )
	{
		ptr = vertexList[i].first;
		cout << i << ":" << vertexList[i].data << " :: ";
		while( ptr != 0 )
		{
			if( ptr->vertex[startEnd] != i )
				swap( startEnd, otherEnd );
			cout << "-> " << vertexList[ptr->vertex[otherEnd]].data;
			ptr = ptr->link[startEnd];
		}
		cout << endl;
	}
}
