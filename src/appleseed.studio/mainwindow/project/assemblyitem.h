
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014-2018 Francois Beaune, The appleseedhq Organization
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

#pragma once

// appleseed.studio headers.
#include "mainwindow/project/basegroupitem.h"
#include "mainwindow/project/entityactions.h"

// appleseed.foundation headers.
#include "foundation/utility/uid.h"

// Qt headers.
#include <QList>
#include <QObject>

namespace appleseed {
namespace studio {

class AssemblyItem
  : public BaseGroupItem
{
    Q_OBJECT

  public:
    AssemblyItem(
        EntityEditorContext&        editor_context,
        renderer::Assembly&         assembly,
        renderer::BaseGroup&        parent,
        BaseGroupItem*              parent_item);

    ~AssemblyItem() override;

    QMenu* get_single_item_context_menu() const override;

    void instantiate(const std::string& name);

  private:
    friend class EntityInstantiationAction<AssemblyItem>;
    friend class EntityDeletionAction<AssemblyItem>;

    renderer::Assembly&             m_assembly;
    const foundation::UniqueID      m_assembly_uid;
    renderer::BaseGroup&            m_parent;
    BaseGroupItem*                  m_parent_item;

    void slot_instantiate() override;
    void do_instantiate(const std::string& name);

    void delete_multiple(const QList<ItemBase*>& items) override;
    void do_delete();
};

}   // namespace studio
}   // namespace appleseed
