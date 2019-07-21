
#
# This source file is part of appleseed.
# Visit https://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2018 Sergo Pogosyan, The appleseedhq Organization
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


#--------------------------------------------------------------------------------------------------
# Source files.
#--------------------------------------------------------------------------------------------------

set (sources
    Alembic/Ogawa/All.h
    Alembic/Ogawa/Foundation.h
    Alembic/Ogawa/IArchive.cpp
    Alembic/Ogawa/IArchive.h
    Alembic/Ogawa/IData.cpp
    Alembic/Ogawa/IData.h
    Alembic/Ogawa/IGroup.cpp
    Alembic/Ogawa/IGroup.h
    Alembic/Ogawa/IStreams.cpp
    Alembic/Ogawa/IStreams.h
    Alembic/Ogawa/OArchive.cpp
    Alembic/Ogawa/OArchive.h
    Alembic/Ogawa/OData.cpp
    Alembic/Ogawa/OData.h
    Alembic/Ogawa/OGroup.cpp
    Alembic/Ogawa/OGroup.h
    Alembic/Ogawa/OStream.cpp
    Alembic/Ogawa/OStream.h
    Alembic/Util/Exception.h
    Alembic/Util/Export.h
    Alembic/Util/Foundation.h
    Alembic/Util/PlainOldDataType.h
)
list (APPEND ogawa_sources
    ${sources}
)
source_group ("" FILES
    ${sources}
)


#--------------------------------------------------------------------------------------------------
# Target.
#--------------------------------------------------------------------------------------------------

add_library (Ogawa STATIC
    ${ogawa_sources}
)

set_target_properties (Ogawa PROPERTIES FOLDER "Third Parties")

apply_static_lib_linker_flags (Ogawa)


#--------------------------------------------------------------------------------------------------
# Include paths.
#--------------------------------------------------------------------------------------------------

include_directories (
    .
)
