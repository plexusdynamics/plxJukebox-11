#pragma once



#include <Python.h>

#include "FileItem.h"

#define ListItem_Check(op) PyObject_TypeCheck(op, &ListItem_Type)
#define ListItem_CheckExact(op) ((op)->ob_type == &ListItem_Type)

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  extern PyTypeObject ListItem_Type;

  typedef struct {
    PyObject_HEAD
    CFileItemPtr item;
  } ListItem;

  extern ListItem* ListItem_FromString(std::string strLabel);

  void initListItem_Type();
}

#ifdef __cplusplus
}
#endif
