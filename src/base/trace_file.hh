#ifndef __TRACE_FILE_HH__
#define __TRACE_FILE_HH__

#include <fstream>
#include <iostream>
#include <string>

/* enable trace file output */
//#define R_TRACE
#define CACHE_MISS_OUTPUT

/* define the interval size, default is 100M, by shen */
#define INTERVAL_SIZE   100000000

//extern bool maxInstsToDump;

/* trace file, by shen */
class TraceFile {
public:
#ifdef R_TRACE
	std::ofstream memTraceFile;
#endif

#ifdef CACHE_MISS_OUTPUT
	std::ofstream dumpFile;
#endif

	TraceFile(std::string path) {
#ifdef R_TRACE
		/* open trace file, by shen */
    	memTraceFile.open(path, std::ios::out);
    	if (memTraceFile.fail()) 
    		std::cout << "can not open the Trace file!!" << std::endl;

    	memTraceFile << "interval" << std::endl;
#endif

#ifdef CACHE_MISS_OUTPUT
    	dumpFile.open(path, std::ios::out | std::ios::trunc);
    	if (dumpFile.fail())
    		std::cout << "can not open the dump file!!" << std::endl;
#endif
	}

	~TraceFile() {
#ifdef R_TRACE
		memTraceFile.close();
#endif

#ifdef CACHE_MISS_OUTPUT
		dumpFile.close();
#endif
	}
};

static TraceFile traceFile("./m5out/branch-miss.dump");

#endif