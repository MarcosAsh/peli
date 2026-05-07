import ctypes
import traceback
from cpython cimport Py_INCREF, Py_DECREF
from numbers import Number, Integral
from ..base import string_types
# (minjie): Node and class module are not used in PELI.
# from ..node_generic import convert_to_node, NodeGeneric
from ..runtime_ctypes import PELIType, PELIContext, PELIByteArray


cdef void peli_callback_finalize(void* fhandle):
    local_pyfunc = <object>(fhandle)
    Py_DECREF(local_pyfunc)

cdef int peli_callback(PELIValue* args,
                      int* type_codes,
                      int num_args,
                      PELIRetValueHandle ret,
                      void* fhandle) with gil:
    cdef list pyargs
    cdef PELIValue value
    cdef int tcode
    local_pyfunc = <object>(fhandle)
    pyargs = []
    for i in range(num_args):
        value = args[i]
        tcode = type_codes[i]
        # (minjie): Node and class module are not used in PELI.
        #if (tcode == kNodeHandle or
        if (tcode == kFuncHandle or
            tcode == kModuleHandle or
            tcode > kExtBegin):
            CALL(PELICbArgToReturn(&value, tcode))

        if tcode != kArrayHandle:
            pyargs.append(make_ret(value, tcode))
        else:
            pyargs.append(c_make_array(value.v_handle, True))
    try:
        rv = local_pyfunc(*pyargs)
    except Exception:
        msg = traceback.format_exc()
        PELIAPISetLastError(c_str(msg))
        return -1
    if rv is not None:
        if isinstance(rv, tuple):
            raise ValueError("PackedFunction can only support one return value")
        temp_args = []
        make_arg(rv, &value, &tcode, temp_args)
        CALL(PELICFuncSetReturn(ret, &value, &tcode, 1))
    return 0


def convert_to_peli_func(object pyfunc):
    """Convert a python function to PELI function

    Parameters
    ----------
    pyfunc : python function
        The python function to be converted.

    Returns
    -------
    pelifunc: peli.Function
        The converted peli function.
    """
    cdef PELIFunctionHandle chandle
    Py_INCREF(pyfunc)
    CALL(PELIFuncCreateFromCFunc(peli_callback,
                                <void*>(pyfunc),
                                peli_callback_finalize,
                                &chandle))
    ret = _CLASS_FUNCTION(None, False)
    (<FunctionBase>ret).chandle = chandle
    return ret


cdef inline int make_arg(object arg,
                         PELIValue* value,
                         int* tcode,
                         list temp_args) except -1:
    """Pack arguments into c args peli call accept"""
    cdef unsigned long long ptr
    # (minjie): Node and class module are not used in PELI.
    #if isinstance(arg, NodeBase):
    #    value[0].v_handle = (<NodeBase>arg).chandle
    #    tcode[0] = kNodeHandle
    if isinstance(arg, NDArrayBase):
        value[0].v_handle = (<NDArrayBase>arg).chandle
        tcode[0] = (kNDArrayContainer if
                    not (<NDArrayBase>arg).c_is_view else kArrayHandle)
    elif isinstance(arg, _PELI_COMPATS):
        ptr = arg._peli_handle
        value[0].v_handle = (<void*>ptr)
        tcode[0] = arg.__class__._peli_tcode
    elif isinstance(arg, (int, long)):
        value[0].v_int64 = arg
        tcode[0] = kInt
    elif isinstance(arg, float):
        value[0].v_float64 = arg
        tcode[0] = kFloat
    elif isinstance(arg, str):
        tstr = c_str(arg)
        value[0].v_str = tstr
        tcode[0] = kStr
        temp_args.append(tstr)
    elif arg is None:
        value[0].v_handle = NULL
        tcode[0] = kNull
    elif isinstance(arg, Number):
        value[0].v_float64 = arg
        tcode[0] = kFloat
    elif isinstance(arg, PELIType):
        tstr = c_str(str(arg))
        value[0].v_str = tstr
        tcode[0] = kStr
        temp_args.append(tstr)
    elif isinstance(arg, PELIContext):
        value[0].v_ctx = (<DLContext*>(
            <unsigned long long>ctypes.addressof(arg)))[0]
        tcode[0] = kPELIContext
    elif isinstance(arg, bytearray):
        arr = PELIByteArray()
        arr.data = ctypes.cast(
            (ctypes.c_byte * len(arg)).from_buffer(arg),
            ctypes.POINTER(ctypes.c_byte))
        arr.size = len(arg)
        value[0].v_handle = <void*>(
            <unsigned long long>ctypes.addressof(arr))
        tcode[0] = kBytes
        temp_args.append(arr)
    elif isinstance(arg, string_types):
        tstr = c_str(arg)
        value[0].v_str = tstr
        tcode[0] = kStr
        temp_args.append(tstr)
    # (minjie): Node and class module are not used in PELI.
    #elif isinstance(arg, (list, tuple, dict, NodeGeneric)):
    #    arg = convert_to_node(arg)
    #    value[0].v_handle = (<NodeBase>arg).chandle
    #    tcode[0] = kNodeHandle
    #    temp_args.append(arg)
    #elif isinstance(arg, _CLASS_MODULE):
    #    value[0].v_handle = c_handle(arg.handle)
    #    tcode[0] = kModuleHandle
    elif isinstance(arg, FunctionBase):
        value[0].v_handle = (<FunctionBase>arg).chandle
        tcode[0] = kFuncHandle
    elif isinstance(arg, ctypes.c_void_p):
        value[0].v_handle = c_handle(arg)
        tcode[0] = kHandle
    elif callable(arg):
        arg = convert_to_peli_func(arg)
        value[0].v_handle = (<FunctionBase>arg).chandle
        tcode[0] = kFuncHandle
        temp_args.append(arg)
    else:
        raise TypeError("Don't know how to handle type %s" % type(arg))
    return 0

cdef inline bytearray make_ret_bytes(void* chandle):
    handle = ctypes_handle(chandle)
    arr = ctypes.cast(handle, ctypes.POINTER(PELIByteArray))[0]
    size = arr.size
    res = bytearray(size)
    rptr = (ctypes.c_byte * size).from_buffer(res)
    if not ctypes.memmove(rptr, arr.data, size):
        raise RuntimeError('memmove failed')
    return res

cdef inline object make_ret(PELIValue value, int tcode):
    """convert result to return value."""
    # (minjie): Node and class module are not used in PELI.
    #if tcode == kNodeHandle:
    #    return make_ret_node(value.v_handle)
    if tcode == kNull:
        return None
    elif tcode == kInt:
        return value.v_int64
    elif tcode == kFloat:
        return value.v_float64
    elif tcode == kNDArrayContainer:
        return c_make_array(value.v_handle, False)
    elif tcode == kStr:
        return py_str(value.v_str)
    elif tcode == kBytes:
        return make_ret_bytes(value.v_handle)
    elif tcode == kHandle:
        return ctypes_handle(value.v_handle)
    elif tcode == kPELIContext:
        return PELIContext(value.v_ctx.device_type, value.v_ctx.device_id)
    # (minjie): Node and class module are not used in PELI.
    #elif tcode == kModuleHandle:
    #    return _CLASS_MODULE(ctypes_handle(value.v_handle))
    elif tcode == kFuncHandle:
        fobj = _CLASS_FUNCTION(None, False)
        (<FunctionBase>fobj).chandle = value.v_handle
        return fobj
    elif tcode in _PELI_EXT_RET:
        return _PELI_EXT_RET[tcode](ctypes_handle(value.v_handle))

    raise ValueError("Unhandled type code %d" % tcode)


cdef inline int FuncCall3(void* chandle,
                          tuple args,
                          int nargs,
                          PELIValue* ret_val,
                          int* ret_tcode) except -1:
    cdef PELIValue[3] values
    cdef int[3] tcodes
    nargs = len(args)
    temp_args = []
    for i in range(nargs):
        make_arg(args[i], &values[i], &tcodes[i], temp_args)
    CALL(PELIFuncCall(chandle, &values[0], &tcodes[0],
                     nargs, ret_val, ret_tcode))
    return 0

cdef inline int FuncCall(void* chandle,
                         tuple args,
                         PELIValue* ret_val,
                         int* ret_tcode) except -1:
    cdef int nargs
    nargs = len(args)
    if nargs <= 3:
        FuncCall3(chandle, args, nargs, ret_val, ret_tcode)
        return 0

    cdef vector[PELIValue] values
    cdef vector[int] tcodes
    values.resize(max(nargs, 1))
    tcodes.resize(max(nargs, 1))
    temp_args = []
    for i in range(nargs):
        make_arg(args[i], &values[i], &tcodes[i], temp_args)
    CALL(PELIFuncCall(chandle, &values[0], &tcodes[0],
                     nargs, ret_val, ret_tcode))
    return 0


cdef inline int ConstructorCall(void* constructor_handle,
                                int type_code,
                                tuple args,
                                void** handle) except -1:
    """Call contructor of a handle function"""
    cdef PELIValue ret_val
    cdef int ret_tcode
    FuncCall(constructor_handle, args, &ret_val, &ret_tcode)
    assert ret_tcode == type_code
    handle[0] = ret_val.v_handle
    return 0


cdef class FunctionBase:
    cdef PELIFunctionHandle chandle
    cdef int is_global

    cdef inline _set_handle(self, handle):
        if handle is None:
            self.chandle = NULL
        else:
            self.chandle = c_handle(handle)

    property is_global:
        def __get__(self):
            return self.c_is_global != 0

        def __set__(self, value):
            self.c_is_global = value

    property handle:
        def __get__(self):
            if self.chandle == NULL:
                return None
            else:
                return ctypes.cast(<unsigned long long>self.chandle, ctypes.c_void_p)
        def __set__(self, value):
            self._set_handle(value)

    def __init__(self, handle, is_global):
        self._set_handle(handle)
        self.c_is_global = is_global

    def __dealloc__(self):
        if self.is_global == 0:
            CALL(PELIFuncFree(self.chandle))

    def __call__(self, *args):
        cdef PELIValue ret_val
        cdef int ret_tcode
        FuncCall(self.chandle, args, &ret_val, &ret_tcode)
        return make_ret(ret_val, ret_tcode)

_CLASS_FUNCTION = None
_CLASS_MODULE = None

def _set_class_module(module_class):
    """Initialize the module."""
    global _CLASS_MODULE
    _CLASS_MODULE = module_class

def _set_class_function(func_class):
    global _CLASS_FUNCTION
    _CLASS_FUNCTION = func_class
