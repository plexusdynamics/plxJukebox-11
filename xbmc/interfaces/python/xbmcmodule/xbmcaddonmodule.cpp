

#include <Python.h>

#include "PythonAddon.h"
#include "pyutil.h"


#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  /*****************************************************************
   * start of xbmcaddon methods
   *****************************************************************/
  // put module methods here


  // define c functions to be used in python here
  PyMethodDef xbmcAddonMethods[] = {
    {NULL, NULL, 0, NULL}
  };
  /*****************************************************************
   * end of methods and python objects
   *****************************************************************/

  PyMODINIT_FUNC
  InitAddonTypes(void)
  {
    initAddon_Type();

    if (PyType_Ready(&Addon_Type) < 0)
      return;
  }

  PyMODINIT_FUNC
  DeinitAddonModule(void)
  {
    // no need to Py_DECREF our objects (see InitAddonModule()) as they were created only
    // so that they could be added to the module, which steals a reference.
  }

  PyMODINIT_FUNC
  InitAddonModule(void)
  {
    Py_INCREF(&Addon_Type);

    // init general xbmcaddon modules
    PyObject* pXbmcAddonModule;
    pXbmcAddonModule = Py_InitModule((char*)"xbmcaddon", xbmcAddonMethods);
    if (pXbmcAddonModule == NULL) return;

    PyModule_AddObject(pXbmcAddonModule, (char*)"Addon", (PyObject*)&Addon_Type);

    // constants
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__author__", (char*)PY_XBMC_AUTHOR);
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__date__", (char*)"1 May 2010");
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__version__", (char*)"1.0");
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__credits__", (char*)PY_XBMC_CREDITS);
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__platform__", (char*)PY_XBMC_PLATFORM);
  }
}

#ifdef __cplusplus
}
#endif
