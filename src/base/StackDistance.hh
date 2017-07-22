#pragma once

#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <list>
#include <float.h>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <time.h>
#include <stdio.h>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/random.hpp>
#include <boost/math/special_functions/gamma.hpp>

//#define REUSE
#define STACK
//#define SAMPLE

#define MISS_BAR 1024 * 2

namespace sdd {

/* to calculate C(a, k) / C(b, k) */
inline double combinationRatio(int b, int a, int k)
{
	if (b < a || !a || !b || !k)
		throw std::exception();
	
	/* This is for the condition that wouldn't occur. */
	if (k > a) return 0.0;


	int loops = b - a;
	double result = 1.0;
	while (loops--)
		result *= (double)(b - k) / (b--);

	if (result <= DBL_MIN)
		throw std::exception();

	return result;
}

/* calculate log2(s) + 1 */
template<class T>
inline T log2p1(T s)
{
	T result = 0;
	while (s) {
		s >>= 1;
		++result;
	}

	return result;
}

/* for recording distribution into a Histogram, 
   Accur is the accuracy of transforming calculation */
template <class B = int64_t>
class Histogram
{
	B * bins;
	B samples;
	int size;

public:
	Histogram() : bins(nullptr), samples(0), size(0) {};

	~Histogram()
	{
    	delete [] bins;
	}

	void allocateBins(int _size)
	{
    	size = _size;
    	bins = new B[size];
    	/* init bins to 0 */
    	for (int i = 0; i < size; ++i)
        	bins[i] = 0;
	
    	std::cout << "size of bins " << size << std::endl;
	}

	void clear()
	{
    	delete [] bins;
    	samples = 0;
    	/* when clear bins, the size keeps unchanged */
    	allocateBins(size);
	}

	void sample(B x);

	void print(std::ofstream & file)
	{
    	//file.write((char *)bins, sizeof(B) * size);
    	for (int i = 0; i < size; ++i) {
    		file << bins[i] << " ";
    	}
    	file << "\n";
	}
};

template <class Accur>
class AvlNode
{
	//int holes;
	friend class AvlTreeStack;
	/* this is the num of holes of this tree,
	including the holes of subtrees and the self interval. */
	int holes;
	/* this is the num of holes of entire right subtree. */
	int rHoles;
	std::pair<Accur, Accur> interval;
	int height;
	AvlNode<Accur> * left;
	AvlNode<Accur> * right;

public:
	AvlNode(Accur & a);

	AvlNode(AvlNode<Accur> & n);

	~AvlNode();

	static int getHeight(AvlNode<Accur> * & node)
	{
		return node ? node->height : -1;
	}

	void updateHeight();

	void updateHoles();
};

/* for calculating stack distance distribution via AVL Tree, with no sampling*/
class AvlTreeStack
{
	std::map <uint64_t, long> addrMap;
	AvlNode<long> * root;
	/* the index of refs in memory trace */
	long index;
	/* holes between current ref and last ref with same address */
	int curHoles;

public:
	AvlTreeStack(long & v);

	AvlTreeStack();

	~AvlTreeStack() { destroy(root); }

	void destroy(AvlNode<long> * & tree);

	void clear();

	void insert(AvlNode<long> * & tree, long & v);

	void insert(long & a);

	/*AvlNode<long> * & find(AvlNode<long> * & tree, int & v)
	{
	if (!tree)
	return nullptr;

	if (v < tree->holes)
	find(tree->left, v);
	else if (v > tree->holes)
	find(tree->right, v);
	else
	return tree;
	}*/

	/* find the minimal interval node */
	AvlNode<long> * & findMin(AvlNode<long> * & tree);
	
	/* find the maximal interval node */
	AvlNode<long> * & findMax(AvlNode<long> * & tree);

	void remove(AvlNode<long> * & tree, std::pair<long, long> & inter);

	void rotate(AvlNode<long> * & tree);

	void doubleRotate(AvlNode<long> * & tree);

	void balance(AvlNode<long> * & tree);

	void calStackDist(uint64_t addr, Histogram<> & hist);
};

/* do reuse distance statistics */
class ReuseDist
{
	std::map<uint64_t, long> addrMap;
	long index;

public:
	ReuseDist() {};
	~ReuseDist() {};

	void calReuseDist(uint64_t addr, Histogram<> & hist);
};

/* stack distance staticstics with sampling */
class SampleStack
{
	long index;

	typedef std::unordered_set<uint64_t> AddrSet;
	/* each watchpoint has a set to keep the unique mem refs */
	std::unordered_map<uint64_t, AddrSet> addrTable;
	int randNum;
	int sampleCounter;
	int expectSamples;
	int hibernInter;
	int sampleInter;
	long statusCounter;
	bool isSampling;

public:
	SampleStack();

	/* to generate a random number */
	int genRandom();

	void calStackDist(uint64_t addr, Histogram<> & hist);
};

}; // namespace sdd