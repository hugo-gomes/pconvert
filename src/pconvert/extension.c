#include "stdafx.h"

PyObject *extension_register(PyObject *self, PyObject *args) {
    Py_RETURN_NONE;
};

PyObject *extension_unregister(PyObject *self, PyObject *args) {
    Py_RETURN_NONE;
};

PyObject *extension_blend_images(PyObject *self, PyObject *args, PyObject *kwargs) {
    int run_inline;
    char demultiply, source_over;
    char *bottom_path, *top_path, *target_path, *algorithm;
    PyObject *is_inline;
    struct pcv_image bottom, top;
    char *kwlist[] = {
        "bottom_path",
        "top_path",
        "target_path",
        "algorithm",
        "is_inline",
        NULL
    };

    bottom_path = NULL;
    top_path = NULL;
    target_path = NULL;
    algorithm = NULL;
    is_inline = NULL;

    if(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "sss|sO",
        &bottom_path,
        &top_path,
        &target_path,
        &algorithm,
        &is_inline
    ) == 0) { return NULL; }

    algorithm = algorithm == NULL ? "multiplicative" : algorithm;
    run_inline = is_inline == NULL ? 0 : PyBool_Check(is_inline);
    demultiply = is_multiplied(algorithm);
    source_over = strcmp(algorithm, "source_over") == 0;

    Py_BEGIN_ALLOW_THREADS;
    VALIDATE_A(read_png(bottom_path, demultiply, &bottom), Py_BLOCK_THREADS Py_RETURN_NONE);
    VALIDATE_A(read_png(top_path, demultiply, &top), Py_BLOCK_THREADS Py_RETURN_NONE);
    if(source_over == TRUE) {
        VALIDATE_A(blend_images_fast(&bottom, &top, algorithm, NULL), Py_BLOCK_THREADS Py_RETURN_NONE);
    } else if(run_inline == TRUE) {
        VALIDATE_A(blend_images_i(&bottom, &top, algorithm, NULL), Py_BLOCK_THREADS Py_RETURN_NONE);
    } else {
        VALIDATE_A(blend_images(&bottom, &top, algorithm, NULL), Py_BLOCK_THREADS Py_RETURN_NONE);
    }
    VALIDATE_A(write_png(&bottom, demultiply, target_path), Py_BLOCK_THREADS Py_RETURN_NONE);
    VALIDATE_A(release_image(&top), Py_BLOCK_THREADS Py_RETURN_NONE);
    VALIDATE_A(release_image(&bottom), Py_BLOCK_THREADS Py_RETURN_NONE);
    Py_END_ALLOW_THREADS;

    Py_RETURN_NONE;
};

PyObject *extension_blend_multiple(PyObject *self, PyObject *args, PyObject *kwargs) {
    int run_inline;
    char demultiply, source_over, use_algorithms;
    char *bottom_path, *top_path, *target_path, *algorithm = NULL;
    struct pcv_image bottom, top;
    PyObject *paths, *iterator, *iteratorAlgorithms, *element, *first, *second,
        *is_inline, *algorithms, *algorithm_o;
    Py_ssize_t size, algorithms_size;
    char *kwlist[] = {
        "paths",
        "target_path",
        "algorithm",
        "algorithms",
        "is_inline",
        NULL
    };

#if PY_MAJOR_VERSION >= 3
    PyObject *encoded;
#endif

    paths = NULL;
    target_path = NULL;
    algorithm = NULL;
    algorithms = NULL;
    is_inline = NULL;

    if(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "Os|sOO",
        kwlist,
        &paths,
        &target_path,
        &algorithm,
        &algorithms,
        &is_inline
    ) == 0) { return NULL; }

    /* tries to determine the target values for the optional parameters
    of the current function (fallback operations) */
    algorithm = algorithm == NULL ? "multiplicative" : algorithm;
    run_inline = is_inline == NULL ? 0 : PyBool_Check(is_inline);

    /* determines the boolean/flag values for both the multiplied entitlement,
    if the algorithm is a source over one and if the algorithms sequence should
    be used to determine the kind of blend algorithm to use */
    demultiply = is_multiplied(algorithm);
    source_over = strcmp(algorithm, "source_over") == 0;
    use_algorithms = algorithms == NULL ? FALSE : TRUE;
    run_inline = use_algorithms == TRUE ? FALSE : run_inline;

    /* retrieves the size of the paths sequence that has been provided
    this is relevant to determine the kind of operation to be performed */
    size = PyList_Size(paths);

    /* in case the use algorithms list strategy is used a small set of
    verifications must be performed */
    if(use_algorithms) {
        algorithms_size = PyList_Size(algorithms);
        if(algorithms_size != size - 1) {
            abort_("[extension_blend_multiple] Invalid algorithms list size");
        }
    }

    /* in case the size of the provided sequence of paths
    is just one then a simple re-write operation should be
    performed, read from provided path and write to target path */
    if(size == 1) {
        first = PyList_GetItem(paths, 0);

#if PY_MAJOR_VERSION >= 3
        first = PyUnicode_EncodeFSDefault(first);
        bottom_path = PyBytes_AsString(first);
#else
        bottom_path = PyString_AsString(first);
#endif
        Py_BEGIN_ALLOW_THREADS;
        VALIDATE_A(
            read_png(bottom_path, demultiply, &bottom),
            Py_BLOCK_THREADS Py_RETURN_NONE
        );
        VALIDATE_A(
            write_png(&bottom, demultiply, target_path),
            Py_BLOCK_THREADS Py_RETURN_NONE
        );
        VALIDATE_A(
            release_image(&bottom),
            Py_BLOCK_THREADS Py_RETURN_NONE
        );
        Py_END_ALLOW_THREADS;
        Py_RETURN_NONE;
    }

    /* in case there's not enough length in the list to allow
    a proper composition, returns the control flow immediately
    as there's nothing remaining to be done */
    if(size < 2) { Py_RETURN_NONE; }

    /* in case the algorithms sequence should be used then the
    target algorithm for the current composition is retrieved
    from the algorithms list*/
    if(use_algorithms) {
        algorithm_o = PyList_GetItem(algorithms, 0);
#if PY_MAJOR_VERSION >= 3
        algorithm_o = PyUnicode_EncodeFSDefault(algorithm_o);
        algorithm = PyBytes_AsString(algorithm_o);
#else
        algorithm = PyString_AsString(algorithm_o);
#endif
        source_over = strcmp(algorithm, "source_over") == 0;
    }

    /* retrieves the first two elements from the list to serve
    as the initial two images to be used in the first composition */
    first = PyList_GetItem(paths, 0);
    second = PyList_GetItem(paths, 1);

#if PY_MAJOR_VERSION >= 3
    first = PyUnicode_EncodeFSDefault(first);
    second = PyUnicode_EncodeFSDefault(second);
    bottom_path = PyBytes_AsString(first);
    top_path = PyBytes_AsString(second);
#else
    bottom_path = PyString_AsString(first);
    top_path = PyString_AsString(second);
#endif

    Py_BEGIN_ALLOW_THREADS;

    /* validates that both the bottom and the top path are correctly
    read from the current file system, in case an error occurs return
    an invalid (none) value to the caller python code */
    VALIDATE_A(
        read_png(bottom_path, demultiply, &bottom),
        Py_BLOCK_THREADS Py_RETURN_NONE
    );
    VALIDATE_A(
        read_png(top_path, demultiply, &top),
        Py_BLOCK_THREADS Py_RETURN_NONE
    );

    if(source_over == TRUE) {
        VALIDATE_A(
            blend_images_fast(&bottom, &top, algorithm, NULL),
            Py_BLOCK_THREADS Py_RETURN_NONE
        );
    } else if(run_inline == TRUE) {
        VALIDATE_A(
            blend_images_i(&bottom, &top, algorithm, NULL),
            Py_BLOCK_THREADS Py_RETURN_NONE
        );
    } else {
        VALIDATE_A(
            blend_images(&bottom, &top, algorithm, NULL),
            Py_BLOCK_THREADS Py_RETURN_NONE
        );
    }
    VALIDATE_A(release_image(&top), Py_BLOCK_THREADS Py_RETURN_NONE);
    Py_END_ALLOW_THREADS;

#if PY_MAJOR_VERSION >= 3
    Py_DECREF(first);
    Py_DECREF(second);
#endif

    iterator = PyObject_GetIter(paths);

    /* increments the iterator by both the top and the bottom
    paths, as they have already been processed */
    PyIter_Next(iterator);
    PyIter_Next(iterator);

    if(use_algorithms) {
        iteratorAlgorithms = PyObject_GetIter(algorithms);
        PyIter_Next(iteratorAlgorithms);
    }

    /* iterates continuously until the iterator is exhausted and
    there's no more images remaining for the blending */
    while(TRUE) {
        element = PyIter_Next(iterator);
        if(element == NULL) { break; }
#if PY_MAJOR_VERSION >= 3
        encoded = PyUnicode_EncodeFSDefault(element);
        top_path = PyBytes_AsString(encoded);
#else
        top_path = PyString_AsString(element);
#endif

        if(use_algorithms) {
            algorithm_o = PyIter_Next(iteratorAlgorithms);
#if PY_MAJOR_VERSION >= 3
            algorithm_o = PyUnicode_EncodeFSDefault(algorithm_o);
            algorithm = PyBytes_AsString(algorithm_o);
#else
            algorithm = PyString_AsString(algorithm_o);
#endif
            source_over = strcmp(algorithm, "source_over") == 0;
        }

        Py_BEGIN_ALLOW_THREADS;

        VALIDATE_A(
            read_png(top_path, demultiply, &top),
            Py_BLOCK_THREADS Py_RETURN_NONE
        );
        if(source_over == TRUE) {
            VALIDATE_A(
                blend_images_fast(&bottom, &top, algorithm, NULL),
                Py_BLOCK_THREADS Py_RETURN_NONE
            );
        } else if(run_inline == TRUE) {
            VALIDATE_A(
                blend_images_i(&bottom, &top, algorithm, NULL),
                Py_BLOCK_THREADS Py_RETURN_NONE
            );
        } else {
            VALIDATE_A(
                blend_images(&bottom, &top, algorithm, NULL),
                Py_BLOCK_THREADS Py_RETURN_NONE
            );
        }
        VALIDATE_A(release_image(&top), Py_BLOCK_THREADS Py_RETURN_NONE);
        Py_END_ALLOW_THREADS;
        Py_DECREF(element);
#if PY_MAJOR_VERSION >= 3
        Py_DECREF(encoded);
#endif
    }

    Py_DECREF(iterator);

    Py_BEGIN_ALLOW_THREADS;
    VALIDATE_A(
        write_png(&bottom, demultiply, target_path),
        Py_BLOCK_THREADS Py_RETURN_NONE
    );
    VALIDATE_A(
        release_image(&bottom),
        Py_BLOCK_THREADS Py_RETURN_NONE
    );
    Py_END_ALLOW_THREADS;

    Py_RETURN_NONE;
};

PyMethodDef pconvert_functions[3] = {
    {
        "blend_images",
        (PyCFunction) extension_blend_images,
        METH_VARARGS | METH_KEYWORDS,
        NULL
    },
    {
        "blend_multiple",
        (PyCFunction) extension_blend_multiple,
        METH_VARARGS | METH_KEYWORDS,
        NULL
    },
    {
        NULL,
        NULL,
        0,
        NULL
    }
};

#if PY_MAJOR_VERSION >= 3
    struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "pconvert",
        "PNG convertion module",
        -1,
        pconvert_functions,
        NULL,
        NULL,
        NULL,
        NULL,
    };
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pconvert(void) {
    /* allocates space for the module object to hold the
    module to be created */
    PyObject *pconvert_module;

    /* creates the pconvert extension module with the
    functions defined in the previous array, in case
    no module is created the control flow is returned
    immediately to the caller function */
    pconvert_module = PyModule_Create(&moduledef);
    if(pconvert_module == NULL) { return NULL; }

    /* adds a series of constants to the module that are
    going to be exposed to the developer */
    PyModule_AddStringConstant(pconvert_module, "VERSION", PCONVERT_VERSION);

    /* returns the module that has just been created to
    the caller method/function (to be used) */
    return pconvert_module;
}
#else
PyMODINIT_FUNC initpconvert(void) {
    /* allocates space for the module object to hold the
    module to be created */
    PyObject *pconvert_module;

    /* creates the pconvert extension module with the
    functions defined in the previous array */
    pconvert_module = Py_InitModule("pconvert", pconvert_functions);
    if(pconvert_module == NULL) { return; }

    /* adds a series of constants to the module that are
    going to be exposed to the developer */
    PyModule_AddStringConstant(pconvert_module, "VERSION", PCONVERT_VERSION);
}
#endif
