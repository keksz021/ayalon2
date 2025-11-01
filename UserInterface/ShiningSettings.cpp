#include "StdAfx.h"
#include "ShiningSettings.h"

PyObject* addEffect(PyObject* poSelf, PyObject* poArgs) {

int vnum;
char* effectpath;

if(!PyTuple_GetInteger(poArgs, 0, &vnum)) {

return Py_BuildException();

}
if(!PyTuple_GetString(poArgs, 1, &effectpath)) {

return Py_BuildException();

}
if(!shiningdata.count(vnum)){
shiningdata[vnum] = effectpath;
}

return Py_BuildNone();
}





void initShining()
{
static PyMethodDef s_methods[] =
{
{ "Add", addEffect, METH_VARARGS },
{ NULL, NULL },
};

Py_InitModule("Shining", s_methods);
}