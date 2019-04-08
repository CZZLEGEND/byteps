# Copyright 2019 ByteDance Inc. or its affiliates. All Rights Reserved.
# Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

# Load all the necessary MXNet C types.
import ctypes
import os

import mxnet as mx
from mxnet.base import c_str, check_call, string_types

from byteps.common import get_ext_suffix
from byteps.common import BytePSBasics as _BytePSBasics
_basics = _BytePSBasics(__file__, 'c_lib')

# import basic methods
shutdown = _basics.shutdown
size = _basics.size
local_size = _basics.local_size
rank = _basics.rank
local_rank = _basics.local_rank

def init():
    use_mpirun = os.getenv("BYTEPS_INIT_USING_HVD")
    if use_mpirun is not None and use_mpirun != 0:
        import horovod.mxnet as hvd
        hvd.init()
        _basics.init(hvd.rank(), hvd.local_rank(), hvd.size(), hvd.local_size())
    else:
        _basics.init()

dll_path = os.path.join(os.path.dirname(__file__),
                        'c_lib' + get_ext_suffix())
MPI_MXNET_LIB_CTYPES = ctypes.CDLL(dll_path, ctypes.RTLD_GLOBAL)


def push_gradients(tensor, version=0, priority=0, name=None):
    """
    A function that performs pushing gradients

    The push operation is keyed by the name. If name is not provided, an
    incremented auto-generated name is used. The tensor type and shape must be
    the same on all BytePS processes for a given name. The reduction will not
    start until all processes are ready to send and receive the tensor.

    This acts as a thin wrapper around an autograd function.  If your input
    tensor requires gradients, then callings this function will allow gradients
    to be computed and backpropagated.

    Arguments:
        tensor: A tensor to average and sum.
        average: A flag indicating whether to compute average or summation,
                 defaults to average.
        name: A name of the reduction operation.

    Returns:
        None
    """

    c_in = tensor.handle
    if isinstance(name, string_types):
        check_call(MPI_MXNET_LIB_CTYPES.byteps_mxnet_push_async(c_in,
                   c_str(name), ctypes.c_int(version), ctypes.c_int(priority)))
    else:
        check_call(MPI_MXNET_LIB_CTYPES.byteps_mxnet_push_async(c_in,
                   name, ctypes.c_int(version), ctypes.c_int(priority)))

    return


def pull_gradients(tensor, version=0, priority=0, name=None):
    """
    A function that performs pulling gradients.

    The pull operation is keyed by the name. If name is not provided, an
    incremented auto-generated name is used. The tensor type and shape must be
    the same on all BytePS processes for a given name. The reduction will not
    start until all processes are ready to send and receive the tensor.

    This acts as a thin wrapper around an autograd function.  If your input
    tensor requires gradients, then callings this function will allow gradients
    to be computed and backpropagated.

    Arguments:
        tensor: A tensor to average and sum.
        average: A flag indicating whether to compute average or summation,
                 defaults to average.
        name: A name of the reduction operation.

    Returns:
        None
    """

    c_out = tensor.handle
    if isinstance(name, string_types):
        check_call(MPI_MXNET_LIB_CTYPES.byteps_mxnet_pull_async(c_out,
                   c_str(name), ctypes.c_int(version), ctypes.c_int(priority)))
    else:
        check_call(MPI_MXNET_LIB_CTYPES.byteps_mxnet_pull_async(c_out,
                   name, ctypes.c_int(version), ctypes.c_int(priority)))
    return