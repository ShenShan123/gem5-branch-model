#ifndef CALCCRITICALPATH_H
#define CALCCRITICALPATH_H

#include "base/types.hh"
//#include "cpu/o3/dep_graph.hh"
#include "cpu/o3/comm.hh"
#include "cpu/o3/fu_pool.hh"
#include "params/DerivO3CPU.hh"
#include <list>
#include <stack>
#include <climits>
/* we try to use Cycles in base/type.hh, but Cycles is actually uint64_t, 
   we use dist[] with negative initiactions. */ 
typedef int Latency;

class AdjNode
{
	typedef unsigned int InstIdx;
	InstIdx v;
	Latency weight;

public:
	bool valid;

	AdjNode() : valid(false) {};

	AdjNode(const InstIdx &  _v, const Latency & _w) : valid(true) { v = _v; weight = _w; }
	
	~AdjNode(){}
	
	InstIdx getV() const { return v; }
	
	Latency getWeight() const { return weight; }

	AdjNode & operator=(const AdjNode & node)
	{
		v = node.getV();
		weight = node.getWeight();
		valid = node.valid;
		return *this;
	}
};

template<class Impl>
class CriticalPath {
	typedef typename Impl::DynInstPtr DynInstPtr;
	typedef std::vector<AdjNode> Graph;
	typedef unsigned int InstIdx;

	int numInstsInRob;
	int numPhysRegs;
	/* dependent graph */
    Graph * dependGraph;
    /* keep track of which inst uses the registers as a destination */
    AdjNode * regInstPair;

    FUPool * fuPool;

public:
	CriticalPath(DerivO3CPUParams *params);

	~CriticalPath();

	Latency calcCriticalPathLength(const std::list<DynInstPtr> & instsInRob);

	void addToProducers(const DynInstPtr &new_inst, const InstIdx idx);
	
	bool addToDependents(const DynInstPtr &new_inst, const InstIdx idx);

	void topologicalSortUtil(const InstIdx v, std::vector<bool> & visited, std::stack<InstIdx> & stack);

	Latency longestPath(const InstIdx & s);
};

template<class Impl>
CriticalPath<Impl>::CriticalPath(DerivO3CPUParams *params) : numInstsInRob(0), 
	numPhysRegs(params->numPhysIntRegs + params->numPhysFloatRegs + params->numPhysCCRegs), 
	fuPool(params->fuPool) {};

template<class Impl>
CriticalPath<Impl>::~CriticalPath()
{
	delete fuPool;
	if (dependGraph != nullptr)
		delete [] dependGraph;
	if (regInstPair != nullptr)
		delete [] regInstPair;
}

template<class Impl>
Latency CriticalPath<Impl>::calcCriticalPathLength(const std::list<DynInstPtr> & instsInRob)
{
	numInstsInRob = instsInRob.size();
	assert(numInstsInRob);

	/* resize the dependGraph same as instList[tid] size */
	dependGraph = new Graph[numInstsInRob];
	regInstPair = new AdjNode[numPhysRegs];
    //InstIdx jIdx = 0;
    //InstIdx iIdx = 0;
    InstIdx instIdx = 0;
	/* traverse the ROB from head to tail to construct a dependency graph */
	for (auto it = instsInRob.begin(); it != instsInRob.end(); ++it) {
    	//assert(instIdx < numInstsInRob);
		// Look through its source registers (physical regs), and mark any
    	// dependencies.
    	addToDependents(*it, instIdx);

    	// Have this instruction set itself as the producer of its destination
    	// register(s).
    	addToProducers(*it, instIdx);

    	++instIdx;
    	//for (PhysRegIndex dest_reg = 0; dest_reg < (*it)->numDestRegs(); ++dest_reg) {
    		//jIdx = iIdx;
    		//auto jt = it;
    		//++jt;
    		//for (; jt != instsInRob.end(); ++jt) {
    			//++jIdx;
    			//for (PhysRegIndex src_reg = 0; src_reg < (*jt)->numSrcRegs(); ++src_reg) {
    				//if((*jt)->renamedSrcRegIdx(src_reg) == (*it)->renamedDestRegIdx(dest_reg)) {
    					//assert(jIdx < numInstsInRob && iIdx < numInstsInRob);
    					//AdjNode adjNode(jIdx, instLatency(*it));
    					//dependGraph[iIdx].push_back(adjNode);
    				//}
    			//}
    		//}
    	//}
    	//++iIdx;
	}

	Latency criticalPathLength = Latency(0);

	/* traverse all insts, call longestPath() to get the critical path */
	for (InstIdx idx = 0; idx < (InstIdx)numInstsInRob; ++idx) {
		if (dependGraph[idx].empty())
			continue;

		Latency temp = longestPath(idx);
		if (temp > criticalPathLength)
			criticalPathLength = temp;
	}

	//if (criticalPathLength > 200)
		//std::cout << "critical path=" << criticalPathLength << " numInstsInRob=" << numInstsInRob << std::endl;
	//assert(criticalPathLength < 200);

	delete [] dependGraph;
	delete [] regInstPair;

	return criticalPathLength;
}

template <class Impl>
void CriticalPath<Impl>::addToProducers(const DynInstPtr &new_inst, const InstIdx idx)
{
    // Nothing really needs to be marked when an instruction becomes
    // the producer of a register's value, but for convenience a ptr
    // to the producing instruction will be placed in the head node of
    // the dependency links.
    int8_t total_dest_regs = new_inst->numDestRegs();

    for (int dest_reg_idx = 0;
         dest_reg_idx < total_dest_regs;
         dest_reg_idx++)
    {
        PhysRegIndex dest_reg = new_inst->renamedDestRegIdx(dest_reg_idx);

        // Instructions that use the misc regs will have a reg number
        // higher than the normal physical registers.  In this case these
        // registers are not renamed, and there is no need to track
        // dependencies as these instructions must be executed at commit.
        if (dest_reg >= numPhysRegs)
            continue;
        /* call FUPool::getOpLatency to get inst latency */
        AdjNode adjNode(idx, (uint64_t)fuPool->getOpLatency(new_inst->opClass()));
        assert(adjNode.getWeight() < 200 && adjNode.getWeight() >= 0);
        assert(adjNode.valid);
        /* this inst map a reg number */
        regInstPair[dest_reg] = adjNode;
    }
}

template <class Impl>
bool CriticalPath<Impl>::addToDependents(const DynInstPtr &new_inst, const InstIdx idx)
{
    // Loop through the instruction's source registers, adding
    // them to the dependency list if they are not ready.
    int8_t total_src_regs = new_inst->numSrcRegs();
    bool return_val = false;

    for (int src_reg_idx = 0;
         src_reg_idx < total_src_regs;
         src_reg_idx++)
    {
        PhysRegIndex src_reg = new_inst->renamedSrcRegIdx(src_reg_idx);

        // Check the IQ's scoreboard to make sure the register
        // hasn't become ready while the instruction was in flight
        // between stages.  Only if it really isn't ready should
        // it be added to the dependency graph.
        if (src_reg >= numPhysRegs)
            continue;

        /* if this src_reg has a dependent reg of ahead insts, add it to the corresponeding list */
        if (regInstPair[src_reg].valid) {
        	/* the dependents must after producers */
        	assert(regInstPair[src_reg].getV() < idx);
        	/* the weight of node is the inst latency that this consumer depends on */
        	AdjNode adjNode(idx, regInstPair[src_reg].getWeight());
        	/* the producer in regInstPair is the index of dependGraph */
        	dependGraph[regInstPair[src_reg].getV()].push_back(adjNode);
        	// Change the return value to indicate that something
        	// was added to the dependency graph.
        	return_val = true;
        }
    }

    return return_val;
}

template <class Impl>
void CriticalPath<Impl>::topologicalSortUtil(const InstIdx v, std::vector<bool> & visited, std::stack<InstIdx> & stack) 
{
	visited[v] = true; // mark current node as visited
	typename std::vector<AdjNode>::iterator it;
	for (it = dependGraph[v].begin(); it != dependGraph[v].end(); ++it) {
		assert(it->getV() < visited.size());
		if (!visited[it->getV()])
			topologicalSortUtil(it->getV(), visited, stack);
	}

	stack.push(v); // push current vertex to stack
}

template <class Impl>
Latency CriticalPath<Impl>::longestPath(const InstIdx & s){
	std::stack<InstIdx> stack;
	std::vector<Latency> dist(numInstsInRob, INT_MIN);
	std::vector<bool> visited(numInstsInRob, false);

	for (InstIdx i = 0; i < (InstIdx)numInstsInRob; ++i)
		/* we traverse the dependeGraph if the node haven't visited and its adjacents are not empty */
		if (!visited[i] && !dependGraph[i].empty())
			topologicalSortUtil(i, visited, stack);
	
	dist[s] = 0;

	while (!stack.empty()){
		InstIdx u = stack.top();
		stack.pop();
		//std::cout << u << " ";

		if (dist[u] != INT_MIN)
			for (auto it = dependGraph[u].begin(); it != dependGraph[u].end(); ++it)
				if (dist[it->getV()] < dist[u] + it->getWeight()) {
					dist[it->getV()] = dist[u] + it->getWeight();
					assert(dist[it->getV()] >= 0 && dist[it->getV()] < 200);
				}
	}

	// return the longest distance
	Latency longestLength = Latency(0);
	for (InstIdx i = 0; i < (InstIdx)numInstsInRob; i++)
		if (longestLength < dist[i])
			longestLength = dist[i];// shen changed

	//if (longestLength > 1000){
		//for (int i = 0; i < numInstsInRob; ++i)
			//std::cout << "dist[" << i << "]=" << dist[i] << " ";
		//std::cout << std::endl;
	//}

	return longestLength;
}

#endif