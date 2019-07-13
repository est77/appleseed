
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

// Interface header.
#include "assemblyitem.h"

// appleseed.studio headers.
#include "mainwindow/project/assemblycollectionitem.h"
#include "mainwindow/project/assemblyinstanceitem.h"
#include "mainwindow/project/collectionitem.h"
#include "mainwindow/project/entityeditorcontext.h"
#include "mainwindow/project/itemregistry.h"
#include "mainwindow/project/materialcollectionitem.h"
#include "mainwindow/project/objectcollectionitem.h"
#include "mainwindow/project/objectinstanceitem.h"
#include "mainwindow/project/projectbuilder.h"
#include "mainwindow/project/singlemodelcollectionitem.h"
#include "mainwindow/project/texturecollectionitem.h"
#include "mainwindow/project/tools.h"
#include "mainwindow/rendering/renderingmanager.h"
#include "utility/miscellaneous.h"

// appleseed.renderer headers.
#include "renderer/api/entity.h"
#include "renderer/api/project.h"
#include "renderer/api/scene.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/foreach.h"
#include "foundation/utility/uid.h"

// Qt headers.
#include <QMenu>
#include <QMessageBox>
#include <QString>

// Standard headers.
#include <memory>
#include <string>
#include <vector>

using namespace foundation;
using namespace renderer;
using namespace std;

namespace appleseed {
namespace studio {

namespace
{
    const UniqueID g_class_uid = new_guid();
}

AssemblyItem::AssemblyItem(
    EntityEditorContext&    editor_context,
    Assembly&               assembly,
    BaseGroup&              parent,
    BaseGroupItem*          parent_item)
  : BaseGroupItem(editor_context, g_class_uid, assembly)
  , m_assembly(assembly)
  , m_assembly_uid(assembly.get_uid())
  , m_parent(parent)
  , m_parent_item(parent_item)
{
    set_title(QString::fromUtf8(assembly.get_name()));

    set_allow_edition(false);

    m_editor_context.m_item_registry.insert(m_assembly, this);
}

AssemblyItem::~AssemblyItem()
{
    m_editor_context.m_item_registry.remove(m_assembly_uid);
}

QMenu* AssemblyItem::get_single_item_context_menu() const
{
    QMenu* menu = BaseGroupItem::get_single_item_context_menu();

    menu->addSeparator();
    menu->addAction("Instantiate...", this, SLOT(slot_instantiate()));

    menu->addSeparator();
    menu->addAction("Import Objects...", &get_object_collection_item(), SLOT(slot_import_objects()));
    menu->addAction("Import Textures...", &get_texture_collection_item(), SLOT(slot_import_textures()));

    menu->addSeparator();
    menu->addAction("Create Assembly...", &get_assembly_collection_item(), SLOT(slot_create()));
    menu->addAction("Create BSDF...", &get_bsdf_collection_item(), SLOT(slot_create()));
    menu->addAction("Create BSSRDF...", &get_bssrdf_collection_item(), SLOT(slot_create()));
    menu->addAction("Create Color...", &get_color_collection_item(), SLOT(slot_create()));
    menu->addAction("Create EDF...", &get_edf_collection_item(), SLOT(slot_create()));
    menu->addAction("Create Light...", &get_light_collection_item(), SLOT(slot_create()));
    menu->addAction("Create Volume...", &get_volume_collection_item(), SLOT(slot_create()));

    QMenu* submenu = menu->addMenu("Create Material...");
    submenu->addAction("Create Generic Material...", &get_material_collection_item(), SLOT(slot_create_generic()));

    menu->addAction("Create Surface Shader...", &get_surface_shader_collection_item(), SLOT(slot_create()));

    return menu;
}

void AssemblyItem::instantiate(const string& name)
{
    m_editor_context.m_rendering_manager.schedule_or_execute(
        unique_ptr<RenderingManager::IScheduledAction>(
            new EntityInstantiationAction<AssemblyItem>(this, name)));
}

void AssemblyItem::slot_instantiate()
{
    const string instance_name_suggestion =
        make_unique_name(
            string(m_assembly.get_name()) + "_inst",
            m_parent.assembly_instances());

    const string instance_name =
        get_entity_name_dialog(
            treeWidget(),
            "Instantiate Assembly",
            "Assembly Instance Name:",
            instance_name_suggestion);

    if (!instance_name.empty())
        instantiate(instance_name);
}

void AssemblyItem::do_instantiate(const string& name)
{
    auto_release_ptr<AssemblyInstance> assembly_instance(
        AssemblyInstanceFactory::create(
            name.c_str(),
            ParamArray(),
            m_assembly.get_name()));

    m_parent_item->get_assembly_instance_collection_item().add_item(assembly_instance.get());
    m_parent.assembly_instances().insert(assembly_instance);

    m_editor_context.m_project.get_scene()->bump_version_id();
    m_editor_context.m_project_builder.slot_notify_project_modification();
}

namespace
{
    int ask_assembly_deletion_confirmation(const char* assembly_name)
    {
        QMessageBox msgbox;
        msgbox.setWindowTitle("Delete Assembly?");
        msgbox.setIcon(QMessageBox::Question);
        msgbox.setText(QString("You are about to delete the assembly \"%1\" and all its instances.").arg(assembly_name));
        msgbox.setInformativeText("Continue?");
        msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgbox.setDefaultButton(QMessageBox::No);
        return msgbox.exec();
    }
}

namespace
{
    vector<UniqueID> collect_assembly_instances(
        const AssemblyInstanceContainer&    assembly_instances,
        const UniqueID                      assembly_uid)
    {
        vector<UniqueID> collected;

        for (const_each<AssemblyInstanceContainer> i = assembly_instances; i; ++i)
        {
            const Assembly* assembly = i->find_assembly();

            if (assembly && assembly->get_uid() == assembly_uid)
                collected.push_back(i->get_uid());
        }

        return collected;
    }

    void remove_assembly_instances(
        ItemRegistry&                       item_registry,
        BaseGroup&                          base_group,
        const UniqueID                      assembly_uid)
    {
        AssemblyInstanceContainer& assembly_instances = base_group.assembly_instances();

        // Collect the assembly instances to remove.
        const vector<UniqueID> remove_list =
            collect_assembly_instances(assembly_instances, assembly_uid);

        // Remove assembly instances and their corresponding project items.
        for (const_each<vector<UniqueID>> i = remove_list; i; ++i)
        {
            assembly_instances.remove(*i);
            delete item_registry.get_item(*i);
        }

        // Recurse into child assemblies.
        for (each<AssemblyContainer> i = base_group.assemblies(); i; ++i)
            remove_assembly_instances(item_registry, *i, assembly_uid);
    }
}

void AssemblyItem::delete_multiple(const QList<ItemBase*>& items)
{
    m_editor_context.m_rendering_manager.schedule_or_execute(
        unique_ptr<RenderingManager::IScheduledAction>(
            new EntityDeletionAction<AssemblyItem>(
                qlist_static_cast<AssemblyItem*>(items))));
}

void AssemblyItem::do_delete()
{
    if (!allows_deletion())
        return;

    const char* assembly_name = m_assembly.get_name();

    if (ask_assembly_deletion_confirmation(assembly_name) != QMessageBox::Yes)
        return;

    const UniqueID assembly_uid = m_assembly.get_uid();

    // Remove all assembly instances and their corresponding project items.
    remove_assembly_instances(
        m_editor_context.m_item_registry,
        m_parent,
        assembly_uid);

    // Remove and delete the assembly.
    m_parent.assemblies().remove(assembly_uid);

    // Mark the project as modified.
    m_editor_context.m_project_builder.slot_notify_project_modification();

    // Remove and delete the assembly item.
    delete this;
}

}   // namespace studio
}   // namespace appleseed
