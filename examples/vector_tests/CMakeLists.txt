#
#  This file is a part of TiledArray.
#  Copyright (C) 2013  Virginia Tech
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#  Justus Calvin
#  Department of Chemistry, Virginia Tech
#
#  CMakeLists.txt
#  Sep 4, 2013
#

# Create the vector executable

# Add include directories
include_directories(${TiledArray_INCLUDE_DIRS})

# Add the vector executable
add_executable(vector EXCLUDE_FROM_ALL vector.cpp)
set_target_properties(vector PROPERTIES
    COMPILE_FLAGS "${CMAKE_CPP_FLAGS} ${TiledArray_COMPILE_FLAGS}"
    LINK_FLAGS "${TiledArray_LINK_FLAGS}")
target_link_libraries(vector ${TiledArray_LIBRARIES})
add_dependencies(vector External)
add_dependencies(example vector)
