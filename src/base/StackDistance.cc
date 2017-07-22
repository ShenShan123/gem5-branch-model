// StackDistance.cpp : Defines the entry point for the console application.
//
#include "StackDistance.hh"

namespace sdd {

template <class B>
void Histogram<B>::sample(B x)
{
    /* the sample number must less than max size of bins */
    assert(x < size);

    if (!bins[x])
        bins[x] = 1;
    else
        ++bins[x];
    /* calculate the total num of sampling */
    ++samples;
}

template <class Accur>
AvlNode<Accur>::AvlNode(Accur & a) : holes(1), rHoles(0), height(0), left(nullptr), right(nullptr)
{
	interval = std::make_pair(a, a);
}

template <class Accur>
AvlNode<Accur>::AvlNode(AvlNode<Accur> & n) : holes(n.holes), rHoles(n.rHoles), height(n.height), left(n.left), right(n.right)
{
	interval = n.interval;
}

template <class Accur>
AvlNode<Accur>::~AvlNode()
{
	delete left;
	delete right;
	left = nullptr;
	right = nullptr;
}

template <class Accur>
void AvlNode<Accur>::updateHeight()
{
	height = 1 + std::max(getHeight(left), getHeight(right));
}

template <class Accur>
void AvlNode<Accur>::updateHoles()
{
	rHoles = right ? right->holes : 0;

	holes =
		(left ? left->holes : 0) + rHoles +
		int(interval.second - interval.first) + 1;
}

AvlTreeStack::AvlTreeStack(long & v) : index(0), curHoles(0)
{
	root = new AvlNode<long>(v);
}

AvlTreeStack::AvlTreeStack() : root(nullptr), index(0), curHoles(0) {};

void AvlTreeStack::destroy(AvlNode<long> * & tree)
{
	if (!tree)
		return;

	destroy(tree->left);
	destroy(tree->right);
	delete tree;
	tree = nullptr;
}

void AvlTreeStack::clear()
{
	addrMap.clear();
	destroy(root);
	index = 0;
	curHoles = 0;
}

void AvlTreeStack::insert(AvlNode<long> * & tree, long & v) {
	if (!tree) {
		tree = new AvlNode<long>(v);
		return;
	}

	std::pair<long, long> & interval = tree->interval;
	assert(!(v >= interval.first && v <= interval.second));

	AvlNode<long> * & n1 = tree->left;
	AvlNode<long> * & n2 = tree->right;

	if (tree->holes < 0)
		return;

	if (v == interval.first - 1) {
		interval.first = v;
		if (n1) {
			std::pair<long, long> & temp = findMax(n1)->interval;
			if (v == temp.second + 1) {
				interval.first = temp.first;
				remove(n1, temp);
			}
		}
		/* update the holes between current address */
		curHoles += interval.second - v + tree->rHoles;
	}

	else if (v == interval.second + 1) {
		interval.second = v;
		if (n2) {
			std::pair<long, long> & temp = findMin(n2)->interval;
			if (v == temp.first - 1) {
				interval.second = temp.second;
				remove(n2, temp);
			}
		}
		curHoles += tree->rHoles;
	}

	else if (v < interval.first - 1) {
		insert(n1, v);
		curHoles += tree->rHoles + interval.second - interval.first + 1;
	}

	else if (v > tree->interval.second + 1)
		insert(n2, v);

	balance(tree);
}

void AvlTreeStack::insert(long & a) 
{	
	/* everytime insert a address, get its total holes */
	curHoles = 0; 
	insert(root, a);
}

AvlNode<long> * & AvlTreeStack::findMin(AvlNode<long> * & tree)
{
	assert(tree);

	if (tree->left == nullptr)
		return tree;
	else
		return findMin(tree->left);
}

AvlNode<long> * & AvlTreeStack::findMax(AvlNode<long> * & tree)
{
	assert(tree);

	if (tree->right == nullptr)
		return tree;
	else
		return findMax(tree->right);
}

void AvlTreeStack::remove(AvlNode<long> * & tree, std::pair<long, long> & inter)
{
	if (!tree)
		return;

	if (inter.first > tree->interval.first)
		remove(tree->right, inter);
	else if (inter.first < tree->interval.first)
		remove(tree->left, inter);

	/* the tree has two children , 
	   replace its content with the min subnode and delete the min node */
	else if (tree->left && tree->right) {
		AvlNode<long> * & minNode = findMin(tree->right);
		tree->interval = minNode->interval;
		remove(tree->right, tree->interval);
	}
	else {
		AvlNode<long> * old = tree;
		if (!tree->left && !tree->right)
			tree = nullptr;
		else
			tree = new AvlNode<long>(*(tree->left ? tree->left : tree->right));

		delete old;
	}

	balance(tree);
}

void AvlTreeStack::rotate(AvlNode<long> * & tree)
{
	if (!tree)
		return;

	if (tree->holes < 0)
		return;

	AvlNode<long> * n1 = tree->left;
	AvlNode<long> * n2 = tree->right;

	if (n2 && AvlNode<long>::getHeight(n1) < AvlNode<long>::getHeight(n2)) {
		tree->right = n2->left;
		tree->updateHoles();
		tree->updateHeight();

		n2->left = tree;
		n2->updateHoles();
		n2->updateHeight();

		tree = n2;
	}
	else if (n1 && AvlNode<long>::getHeight(n1) > AvlNode<long>::getHeight(n2)) {
		tree->left = n1->right;
		tree->updateHoles();
		tree->updateHeight();

		n1->right = tree;
		n1->updateHoles();
		n1->updateHeight();

		tree = n1;
	}
}

void AvlTreeStack::doubleRotate(AvlNode<long> * & tree)
{
	if (!tree)
		return;

	if (tree->holes < 0)
		return;

	AvlNode<long> * & n1 = tree->left;
	AvlNode<long> * & n2 = tree->right;

	if (AvlNode<long>::getHeight(n1) < AvlNode<long>::getHeight(n2))
		rotate(n2);
	else
		rotate(n1);

	rotate(tree);
}

void AvlTreeStack::balance(AvlNode<long> * & tree)
{
	if (!tree)
		return;

	AvlNode<long> * & n1 = tree->left;
	AvlNode<long> * & n2 = tree->right;
	int err = AvlNode<long>::getHeight(n1) - AvlNode<long>::getHeight(n2);
	if (err > 1) {
		if (AvlNode<long>::getHeight(n1->left) >= AvlNode<long>::getHeight(n1->right))
			rotate(tree);
		else
			doubleRotate(tree);
	}
	else if (err < -1) {
		if (AvlNode<long>::getHeight(n2->left) <= AvlNode<long>::getHeight(n2->right))
			rotate(tree);
		else
			doubleRotate(tree);
	}

	tree->updateHeight();
	tree->updateHoles();
}

void AvlTreeStack::calStackDist(uint64_t addr, Histogram<> & hist)
{
	long & value = addrMap[addr];

	++index;

	/* value is 0 under cold miss */
	if (!value) {
		value = index;
		hist.sample(log2p1(MISS_BAR));
		return;
	}

	/* update b of last reference */
	if (value < index) {
		/* insert a hole */
		insert(value);
		int stackDist = index - value - curHoles - 1;
		/* if the stack distance is large than MISS_BAR, the reference is definitely missed. */
		stackDist = stackDist >= MISS_BAR ? MISS_BAR : stackDist;
		hist.sample(log2p1(stackDist));
	}

	value = index;
}

void ReuseDist::calReuseDist(uint64_t addr, Histogram<> & hist)
{
	long & value = addrMap[addr];

	++index;

	/* value is 0 under cold miss */
	if (!value) {
		hist.sample(MISS_BAR);
		value = index;
		return;
	}

	/* update b of last reference */
	if (value < index) {
		int reuseDist = index - value - 1;
		reuseDist = reuseDist >= MISS_BAR ? MISS_BAR : reuseDist;
		hist.sample(reuseDist);
	}

	value = index;
}

SampleStack::SampleStack() : sampleCounter(0), expectSamples(2000), hibernInter(3000000),
sampleInter(1000000), isSampling(false), statusCounter(0) {};

int SampleStack::genRandom()
{
	// construct a trivial random generator engine from a time-based seed:
	std::chrono::system_clock::rep seed = std::chrono::system_clock::now().time_since_epoch().count();
	static std::mt19937 engine(seed);
	static std::geometric_distribution<int> geom((double)expectSamples / sampleInter);
	int rand = 0;
	/* the random can not be 0 */
	while (!rand)
		rand = geom(engine);

	return rand;
}

void SampleStack::calStackDist(uint64_t addr, Histogram<> & hist)
{
	++sampleCounter;
	++statusCounter;

	/* start sampling interval */
	if (!isSampling && statusCounter == hibernInter) {
		statusCounter = 0;
		sampleCounter = 0;
		isSampling = true;
		randNum = genRandom();
	}
	/* start hibernation interval */
	else if (isSampling && statusCounter == sampleInter) {
		statusCounter = 0;
		isSampling = false;
	}

	/* if we find a same address x in addrTable,
	record its stack distance and the sampling of x is finished */
	auto pos = addrTable.find(addr);
	if (pos != addrTable.end()) {
		hist.sample(pos->second.size() - 1);
		addrTable.erase(addr);
	}

	auto it = addrTable.begin();

	/* make sure the max size of addrTable */
	if (addrTable.size() > expectSamples) {
		hist.sample(MISS_BAR);
		addrTable.erase(it->first);
	}

	/* record unique mem references between the sampled address x */
	for (it = addrTable.begin(); it != addrTable.end(); ) {
		it->second.insert(addr);
		/* if the set of sampled address x is too large,
		erase it from  the table and record as MISS_BAR */
		if (it->second.size() > MISS_BAR) {
			auto eraseIt = it;
			++it;
			hist.sample(MISS_BAR);
			addrTable.erase(eraseIt->first);
		}
		else
			++it;
	}

	/* if it is time to do sampling */
	if (isSampling && sampleCounter == randNum) {
		/* it is a new sampled address */
		assert(!addrTable[addr].size());
		addrTable[addr].insert(0);
		/* reset the sampleCounter and randNum to prepare next sample */
		sampleCounter = 0;
		randNum = genRandom();
		//randNum = 1;
	}
}

}; // namespace sdd