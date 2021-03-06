#include <Python.h>
#include "libinplace_abn.h"
#include <numpy/arrayobject.h>
#include <stdlib.h> /* getenv */
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

// #include <iostream>
#define NUMPY_IMPORT_ARRAY_RETVAL

PyObject *python_libinplace_abn_process_image_function;

void initialize_python_path_inplace()
{
	char* pyPath;
	char* pPath;
	char* inplacePath;
	pyPath = (char *) "PYTHONPATH=";
  	pPath = getenv ("CARMEN_HOME");
	inplacePath = (char *) "/sharedlib/inplace_abn/scripts";
    char * path = (char *) malloc(1 + strlen(pyPath) + strlen(pPath)+ strlen(inplacePath));
	strcpy(path, pyPath);
    strcat(path, pPath);
    strcat(path, inplacePath);
	putenv(path);
}

void
initialize_python_context()
{
	initialize_python_path_inplace();
	Py_Initialize();
	import_array();
	PyObject *python_module = PyImport_ImportModule("run_inplace_abn");
	
	if (PyErr_Occurred())
		        PyErr_Print();

	if (python_module == NULL)
	{
		Py_Finalize();
		exit(printf("Error: The python_module run_test could not be loaded.\nMaybe PYTHONPATH is not set.\n"));
	}

	if (PyErr_Occurred())
		        PyErr_Print();

	PyObject *python_initialize_function = PyObject_GetAttrString(python_module, (char *) "initialize");

	if (PyErr_Occurred())
		        PyErr_Print();
	if (python_initialize_function == NULL || !PyCallable_Check(python_initialize_function))
	{
		Py_Finalize();
		exit (printf("Error: Could not load the python_initialize_function.\n"));
	}
	PyObject *python_arguments = Py_BuildValue("(i)", 640);

	if (PyErr_Occurred())
		        PyErr_Print();

	PyObject_CallObject(python_initialize_function, python_arguments);

	if (PyErr_Occurred())
		        PyErr_Print();

	// Py_DECREF(python_arguments);
	// Py_DECREF(python_initialize_function);

	python_libinplace_abn_process_image_function = PyObject_GetAttrString(python_module, (char *) "inplace_abn_process_image");
	
	if (PyErr_Occurred())
		        PyErr_Print();

	if (python_libinplace_abn_process_image_function == NULL || !PyCallable_Check(python_libinplace_abn_process_image_function))
	{
		// Py_DECREF(python_module);
		Py_Finalize();
		exit (printf("Error: Could not load the inplace_abn_process_image.\n"));
	}

	if (PyErr_Occurred())
		        PyErr_Print();

	printf("Success: Loaded inplace_abn\n");

}

float*
libinplace_abn_process_image(int width, int height, unsigned char *image)
{
	printf("libinplace_abn_process_image\n");
	//create shape for numpy array
	npy_intp dims[3] = {height, width, 3};
	PyObject* numpyArray = PyArray_SimpleNewFromData(3, dims, NPY_UBYTE, image);

	if (PyErr_Occurred())
		        PyErr_Print();

	PyArrayObject* python_result_array = (PyArrayObject*) PyObject_CallFunction(python_libinplace_abn_process_image_function, (char *) "(O)", numpyArray);

	if (PyErr_Occurred())
	        PyErr_Print();

	float *result_array = (float*)PyArray_DATA(python_result_array);

	if (PyErr_Occurred())
        PyErr_Print();

	Py_DECREF(numpyArray);
	//Py_DECREF(python_result_array);

	return result_array;
}

