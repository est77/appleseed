
#
# This source file is part of appleseed.
# Visit http://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2015-2016 Esteban Tovagliari, The appleseedhq Organization
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
# Find Ptex headers and libraries.
#
# This module defines the following variables:
#
#   PTEX_FOUND            True if Ptex was found
#   PTEX_INCLUDE_DIRS     Where to find Ptex header files
#   PTEX_LIBRARIES        List of Ptex libraries to link against
#

include (FindPackageHandleStandardArgs)

find_path (PTEX_INCLUDE_DIR NAMES PtexVersion.h)

find_library (PTEX_LIBRARY NAMES Ptex)

# Handle the QUIETLY and REQUIRED arguments and set PTEX_FOUND.
find_package_handle_standard_args (PTEX DEFAULT_MSG
    PTEX_INCLUDE_DIR
    PTEX_LIBRARY
)

# Set the output variables.
if (PTEX_FOUND)
    set (PTEX_INCLUDE_DIRS ${PTEX_INCLUDE_DIR})
    set (PTEX_LIBRARIES ${PTEX_LIBRARY})
else ()
    set (PTEX_INCLUDE_DIRS)
    set (PTEX_LIBRARIES)
endif ()

mark_as_advanced (
    PTEX_INCLUDE_DIR
    PTEX_LIBRARY
)
