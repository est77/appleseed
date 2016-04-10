
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016 Esteban Tovagliari, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef APPLESEED_RENDERER_KERNEL_INTERSECTION_PATCHKEY_H
#define APPLESEED_RENDERER_KERNEL_INTERSECTION_PATCHKEY_H

// appleseed.foundation.headers.
#include "foundation/platform/types.h"

// Standard headers.
#include <cstddef>

namespace renderer
{


class PatchKey
{
  public:
    // Constructors.
    PatchKey();         // leave all fields uninitialized
    PatchKey(
        const size_t    object_instance_index,
        const size_t    patch_index_object,
        const size_t    patch_index_tree,
        const size_t    patch_pa,
        const size_t    patch_type);

    // Return the index of the object instance within the assembly.
    size_t get_object_instance_index() const;

    // Return the index of the patch within the object.
    size_t get_patch_index_object() const;

    // Return the index of the patch within the tree.
    void set_patch_index_tree(const size_t patch_index_tree);
    size_t get_patch_index_tree() const;

    // Return the primitive attribute index of the patch.
    size_t get_patch_pa() const;

    // Return the patch type
    size_t get_patch_type() const;

  private:
    foundation::uint32  m_object_instance_index;
    foundation::uint32  m_patch_index_object;
    foundation::uint32  m_patch_index_tree;
    foundation::uint16  m_patch_pa;
    foundation::uint16  m_patch_type;
};


//
// PatchKey class implementation.
//

inline PatchKey::PatchKey()
{
}

inline PatchKey::PatchKey(
    const size_t        object_instance_index,
    const size_t        patch_index_object,
    const size_t        patch_index_tree,
    const size_t        patch_pa,
    const size_t        patch_type)
  : m_object_instance_index(static_cast<foundation::uint32>(object_instance_index))
  , m_patch_index_object(static_cast<foundation::uint32>(patch_index_object))
  , m_patch_index_tree(static_cast<foundation::uint32>(patch_index_tree))
  , m_patch_pa(static_cast<foundation::uint16>(patch_pa))
  , m_patch_type(static_cast<foundation::uint16>(patch_type))
{
}

inline size_t PatchKey::get_object_instance_index() const
{
    return static_cast<size_t>(m_object_instance_index);
}

inline size_t PatchKey::get_patch_index_object() const
{
    return static_cast<size_t>(m_patch_index_object);
}

inline void PatchKey::set_patch_index_tree(const size_t patch_index_tree)
{
    m_patch_index_tree = static_cast<foundation::uint32>(patch_index_tree);
}

inline size_t PatchKey::get_patch_index_tree() const
{
    return static_cast<size_t>(m_patch_index_tree);
}

inline size_t PatchKey::get_patch_pa() const
{
    return static_cast<size_t>(m_patch_pa);
}

inline size_t PatchKey::get_patch_type() const
{
    return static_cast<size_t>(m_patch_type);
}

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_KERNEL_INTERSECTION_PATCHKEY_H
