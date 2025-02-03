#include "DelayLoad.h"

std::mutex mtx;
std::condition_variable cv;
bool modelsLoaded = false;