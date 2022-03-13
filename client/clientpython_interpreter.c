#include <stdio.h>
#include <python2.7/Python.h>

int main(int argc, char *argv[])
{
	FILE* fp;

	Py_SetProgramName(argv[0]);
	Py_Initialize();
	PySys_SetArgv(argc, argv);

	fp = fopen("client.py", "r");
	PyRun_SimpleFile(fp, "client.py");

	Py_Finalize();
	return 0;
}