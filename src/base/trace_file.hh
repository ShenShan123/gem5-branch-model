#ifndef __TRACE_FILE_HH__
#define __TRACE_FILE_HH__

#include <fstream>
#include <iostream>
#include <string>
#include "params/System.hh"

/* enable trace file output */
#define SDD_TRACE
//#define CACHE_MISS_OUTPUT

/* define the interval size, default is 100M, by shen */
//#define WINDOW_SIZE   10000000

//extern bool maxInstsToDump;

/* trace file, by shen */
class TraceFile {
public:
	//long windowSize;
#ifdef SDD_TRACE
	std::ofstream dump;
#endif

#ifdef CACHE_MISS_OUTPUT
	std::ofstream dumpFile;
#endif

	TraceFile(std::string path) {
#ifdef SDD_TRACE
		/* open trace file, by shen */
    	dump.open(path, std::ios::out | std::ios::binary);
    	if (dump.fail()) 
    		std::cout << "can not open the Trace file!!" << std::endl;

    	//dump << "start dump" << std::endl;
#endif

#ifdef CACHE_MISS_OUTPUT
    	dumpFile.open(path, std::ios::out | std::ios::trunc);
    	if (dumpFile.fail())
    		std::cout << "can not open the dump file!!" << std::endl;
#endif
	}

	~TraceFile() {
#ifdef SDD_TRACE
		dump.close();
#endif

#ifdef CACHE_MISS_OUTPUT
		dumpFile.close();
#endif
	}
};

static TraceFile traceFile("./m5out/SDD.dump" );

#endif