// handles everything
#include "Application.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


void CheckMemoryLeaks()
{
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtDumpMemoryLeaks();
}

// program entry point
int main()
{
	//_CrtSetBreakAlloc(0);

	Application madeline;
	if (madeline.Init()) {
		if (madeline.Run()) {
			//CheckMemoryLeaks();

			return madeline.Shutdown() ? 0 : 1;
		}
	}

	//CheckMemoryLeaks();
	return 1;
}