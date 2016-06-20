#include "rwops.h"
#include <iostream>
#include <string.h>

using namespace std;
const char* RWopsReadTestFilename = "rwops_read.dat";

char writeFile[] = "hello world";

int main(int argc, char* argv[])
{
	FILE *fp;
	RWops *rw;
	fp = fopen(RWopsReadTestFilename, "a+");
	if(NULL == fp)
	{
		std::cout << "fail to open the file" << std::endl;
		return -1;
	}

	rw = RWFromFP(fp, 1);

	if(NULL == rw)
	{
		std::cerr << "fail in RWFromFP" << std::endl;
		fclose(fp);

		return -1;
	}
	else
	{
		std::cout << "success in RWFromFP" << std::endl;
	}

	RWwrite(rw, writeFile, 1, strlen(writeFile));
	long ret = RWsize(rw);


	std::cout << "The size of the file is " << ret << std::endl;

	return 0;

}
