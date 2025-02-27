/*
 *
 * This file is part of Tulip (http://tulip.labri.fr)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */

// +-------------------------------------------------------------------------+
// | Tulip Python Bindings                                                   |
// | inspired from bindings by the Booggie project development team          |
// | (https://sourceforge.net/projects/booggie/)                             |
// +-------------------------------------------------------------------------+

// ****************************************************
// SIP generic implementation for std::vector<>
// ****************************************************
// This template-based generic implementation is valid only
// if the element type is a SIP-wrapped type. 
// For basic types (int, double, etc.),
// we are forced to provide a specialization.

template<TYPE>
%MappedType std::vector<TYPE> /TypeHint="List[TYPE]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  const sipTypeDef* kpTypeDef = getSipType("TYPE");

  if (!kpTypeDef) {
    return NULL;
  }

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // SIP wrapper.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    TYPE *cpp = new TYPE(sipCpp -> at(i));
    PyObject *pobj;

    // Get the Python wrapper for the Type instance, creating a new
    // one if necessary, and handle any ownership transfer.
    if ((pobj = sipConvertFromNewType(cpp, kpTypeDef, sipTransferObj)) == NULL) {
      // There was an error so garbage collect the Python list.
      Py_XDECREF(l);
      return NULL;
    }

    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, pobj);
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode

  const sipTypeDef* kpTypeDef = getSipType("TYPE");

  if (!kpTypeDef) {
    return 0;
  }

  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!sipCanConvertToType(item, kpTypeDef, SIP_NOT_NONE)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to TYPE");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of TYPE to  std::vector<TYPE>
  std::vector<TYPE> *v = new std::vector<TYPE>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    int state;
    TYPE* p = static_cast<TYPE*>(sipConvertToType(PyList_GET_ITEM(sipPy, i), kpTypeDef, sipTransferObj, SIP_NOT_NONE, &state, sipIsErr));

    if (*sipIsErr) {
      sipReleaseType(p, kpTypeDef, state);
      delete v;
      return 0;
    }

    v->push_back(*p);

    sipReleaseType(p, kpTypeDef, state);
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End
};

template<TYPE*>
%MappedType std::vector<TYPE*> /TypeHint="List[TYPE]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  const sipTypeDef* kpTypeDef = getSipType("TYPE");

  if (!kpTypeDef) {
    return NULL;
  }

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // SIP wrapper.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    PyObject *pobj;

    // Get the Python wrapper for the Type instance, creating a new
    // one if necessary, and handle any ownership transfer.
    if ((pobj = sipConvertFromType(sipCpp -> at(i), kpTypeDef, sipTransferObj)) == NULL) {
      // There was an error so garbage collect the Python list.
      Py_XDECREF(l);
      return NULL;
    }

    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, pobj);
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode

  const sipTypeDef* kpTypeDef = getSipType("TYPE");

  if (!kpTypeDef) {
    return 0;
  }

  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!sipCanConvertToType(item, kpTypeDef, SIP_NOT_NONE)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to TYPE");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of TYPE to std::vector<TYPE*>
  std::vector<TYPE*> *v = new std::vector<TYPE*>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    int state;
    PyObject *item = PyList_GET_ITEM(sipPy, i);
    TYPE* p = static_cast<TYPE*>(sipConvertToType(item, kpTypeDef, NULL, SIP_NOT_NONE, &state, sipIsErr));

    if (*sipIsErr) {
      sipReleaseType(p, kpTypeDef, state);
      delete v;
      return 0;
    }

    sipTransferTo(item, item);
    v->push_back(p);

    sipReleaseType(p, kpTypeDef, state);
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End
};

// ****************************************************
// Specialization for std::vector<double>
// ****************************************************

%MappedType std::vector<double> /TypeHint="List[float]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // wrapped object.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, PyFloat_FromDouble(sipCpp -> at(i)));
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!PyNumber_Check(item)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to double");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of numbers to a std::vector<double>
  std::vector<double> *v = new std::vector<double>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    PyObject *f = PyNumber_Float(PyList_GET_ITEM(sipPy, i));
    v->push_back(PyFloat_AsDouble(f));
    Py_XDECREF(f);
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End
};

// ****************************************************
// Specialization for std::vector<float>
// ****************************************************

%MappedType std::vector<float> /TypeHint="List[float]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // wrapped object.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, PyFloat_FromDouble(sipCpp -> at(i)));
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!PyNumber_Check(item)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to float");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of numbers to a std::vector<double>
  std::vector<float> *v = new std::vector<float>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    PyObject *f = PyNumber_Float(PyList_GET_ITEM(sipPy, i));
    v->push_back(float(PyFloat_AsDouble(f)));
    Py_XDECREF(f);
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End
};


// ****************************************************
// Specialization for std::vector<int>
// ****************************************************

%MappedType std::vector<int> /TypeHint="List[int]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // wrapped object.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, PyLong_FromLong(sipCpp -> at(i)));
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!PyLong_Check(item)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to int");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of integers to a std::vector<int>
  std::vector<int> *v = new std::vector<int>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    v->push_back(int(PyLong_AsLong(PyList_GET_ITEM(sipPy, i))));
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End

};

// ****************************************************
// Specialization for std::vector<unsigned int>
// ****************************************************

%MappedType std::vector<unsigned int> /TypeHint="List[int]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // wrapped object.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, PyLong_FromUnsignedLong(sipCpp -> at(i)));
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!PyLong_Check(item)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to int");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of integers to a std::vector<int>
  std::vector<unsigned int> *v = new std::vector<unsigned int>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    v->push_back(uint(PyLong_AsUnsignedLong(PyList_GET_ITEM(sipPy, i))));
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End

};

// ****************************************************
// Specialization for std::vector<long>
// ****************************************************

%MappedType std::vector<long> /TypeHint="List[int]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // wrapped object.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, PyLong_FromLong(sipCpp -> at(i)));
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!PyLong_Check(item)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to long");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of integers to a std::vector<int>
  std::vector<long> *v = new std::vector<long>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    v->push_back(PyLong_AsLong(PyList_GET_ITEM(sipPy, i)));
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End

};

// ****************************************************
// Specialization for std::vector<unsigned long>
// ****************************************************

%MappedType std::vector<unsigned long> /TypeHint="List[int]"/ {
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // wrapped object.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, PyLong_FromUnsignedLong(sipCpp -> at(i)));
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!PyLong_Check(item)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to unsigned long");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of integers to a std::vector<int>
  std::vector<unsigned long> *v = new std::vector<unsigned long>();
  v->reserve(PyList_GET_SIZE(sipPy));
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    v->push_back(PyLong_AsUnsignedLong(PyList_GET_ITEM(sipPy, i)));
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End

};

// ****************************************************
// Specialization for std::vector<bool>
// ****************************************************

%MappedType std::vector<bool> /TypeHint="List[bool]"/{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  PyObject *l;

  // Create the Python list of the correct length.
  if ((l = PyList_New(sipCpp -> size())) == NULL) {
    return NULL;
  }

  // Go through each element in the C++ instance and convert it to a
  // wrapped object.
  for (size_t i = 0; i < sipCpp -> size(); ++i) {
    // Add the wrapper to the list.
    PyList_SET_ITEM(l, i, PyBool_FromLong(sipCpp -> at(i)));
  }

  // Return the Python list.
  return l;
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy)) {
      return 0;
    }
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
      PyObject *item = PyList_GET_ITEM(sipPy, i);
      if (!PyBool_Check(item)) {
        //printErrorMessage("TypeError : object in list of type " + std::string(item->ob_type->tp_name) + " can not be converted to bool");
        return 0;
      }
    }
    return 1;
  }

  // Convert Python list of booleans to a std::vector<bool>
  std::vector<bool> *v = new std::vector<bool>();
  v->reserve(PyList_GET_SIZE(sipPy));	
  for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    v->push_back(PyList_GET_ITEM(sipPy, i) == Py_True);
  }

  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End

};
