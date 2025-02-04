#ifndef DELAYLOAD_H
#define DELAYLOAD_H

#include <mutex>
#include <condition_variable>

extern std::mutex mtx;
extern std::condition_variable cv;
extern bool modelsLoaded;

#endif
