
#
# This source file is part of appleseed.
# Visit https://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2019 Esteban Tovagliari, The appleseedhq Organization
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#


#
# Find Arnold headers and libraries.
#
# This module defines the following variables:
#
#   ARNOLD_FOUND           True if ARNOLD was found
#   ARNOLD_INCLUDE_DIRS    Where to find ARNOLD header files
#   ARNOLD_LIBRARIES       List of ARNOLD libraries to link against
#

include (FindPackageHandleStandardArgs)

find_path (ARNOLD_INCLUDE_DIR NAMES ai.h)
find_library (ARNOLD_LIBRARY NAMES ai)

# Handle the QUIETLY and REQUIRED arguments and set ARNOLD_FOUND.
find_package_handle_standard_args (ARNOLD DEFAULT_MSG
    ARNOLD_INCLUDE_DIR
    ARNOLD_LIBRARY
)

# Set the output variables.
if (ARNOLD_FOUND)
    set (ARNOLD_INCLUDE_DIRS ${ARNOLD_INCLUDE_DIR})
    set (ARNOLD_LIBRARIES ${ARNOLD_LIBRARY})
else ()
    set (ARNOLD_INCLUDE_DIRS)
    set (ARNOLD_LIBRARIES)
endif ()

mark_as_advanced (
    ARNOLD_INCLUDE_DIR
    ARNOLD_LIBRARY
)
