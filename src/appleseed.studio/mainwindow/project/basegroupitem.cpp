
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
#include "basegroupitem.h"

// appleseed.studio headers.
#include "mainwindow/project/assemblycollectionitem.h"
#include "mainwindow/project/assemblyinstanceitem.h"
#include "mainwindow/project/collectionitem.h"
#include "mainwindow/project/instancecollectionitem.h"
#include "mainwindow/project/materialcollectionitem.h"
#include "mainwindow/project/objectcollectionitem.h"
#include "mainwindow/project/objectinstanceitem.h"
#include "mainwindow/project/singlemodelcollectionitem.h"
#include "mainwindow/project/texturecollectionitem.h"
#include "mainwindow/project/textureinstanceitem.h"

// appleseed.renderer headers.
#include "renderer/api/bsdf.h"
#include "renderer/api/bssrdf.h"
#include "renderer/api/color.h"
#include "renderer/api/edf.h"
#include "renderer/api/entity.h"
#include "renderer/api/light.h"
#include "renderer/api/material.h"
#include "renderer/api/project.h"
#include "renderer/api/scene.h"
#include "renderer/api/scene.h"
#include "renderer/api/shadergroup.h"
#include "renderer/api/surfaceshader.h"
#include "renderer/api/utility.h"
#include "renderer/api/volume.h"

// Qt headers.
#include <QMenu>

using namespace foundation;
using namespace renderer;

namespace appleseed {
namespace studio {

BaseGroupItem::BaseGroupItem(
    EntityEditorContext&    editor_context,
    const UniqueID          class_uid,
    BaseGroup&              base_group)
  : ItemBase(editor_context, class_uid)
{
    add_items(base_group);
}

BaseGroupItem::BaseGroupItem(
    EntityEditorContext&    editor_context,
    const UniqueID          class_uid,
    const QString&          title,
    BaseGroup&              base_group)
  : ItemBase(editor_context, class_uid, title)
{
    add_items(base_group);
}

ItemBase* BaseGroupItem::add_item(renderer::Assembly* assembly)
{
    return m_assembly_collection_item->add_item(assembly);
}

ItemBase* BaseGroupItem::add_item(renderer::AssemblyInstance* assembly_instance)
{
    return m_assembly_instance_collection_item->add_item(assembly_instance);
}

ItemBase* BaseGroupItem::add_item(BSDF* bsdf)
{
    return m_bsdf_collection_item->add_item(bsdf);
}

ItemBase* BaseGroupItem::add_item(BSSRDF* bssrdf)
{
    return m_bssrdf_collection_item->add_item(bssrdf);
}

ItemBase* BaseGroupItem::add_item(ColorEntity* color)
{
    return m_color_collection_item->add_item(color);
}

ItemBase* BaseGroupItem::add_item(EDF* edf)
{
    return m_edf_collection_item->add_item(edf);
}

ItemBase* BaseGroupItem::add_item(Light* light)
{
    return m_light_collection_item->add_item(light);
}

ItemBase* BaseGroupItem::add_item(Material* material)
{
    return m_material_collection_item->add_item(material);
}

ItemBase* BaseGroupItem::add_item(Object* object)
{
    return m_object_collection_item->add_item(object);
}

ItemBase* BaseGroupItem::add_item(ObjectInstance* object_instance)
{
    return m_object_instance_collection_item->add_item(object_instance);
}

ItemBase* BaseGroupItem::add_item(ShaderGroup* shader_group)
{
    return m_shader_group_collection_item->add_item(shader_group);
}

ItemBase* BaseGroupItem::add_item(SurfaceShader* surface_shader)
{
    return m_surface_shader_collection_item->add_item(surface_shader);
}

ItemBase* BaseGroupItem::add_item(Texture* texture)
{
    return m_texture_collection_item->add_item(texture);
}

ItemBase* BaseGroupItem::add_item(TextureInstance* texture_instance)
{
    return m_texture_instance_collection_item->add_item(texture_instance);
}

ItemBase* BaseGroupItem::add_item(Volume* volume)
{
    return m_volume_collection_item->add_item(volume);
}

AssemblyCollectionItem& BaseGroupItem::get_assembly_collection_item() const
{
    return *m_assembly_collection_item;
}

BaseGroupItem::AssemblyInstanceCollectionItem& BaseGroupItem::get_assembly_instance_collection_item() const
{
    return *m_assembly_instance_collection_item;
}

BaseGroupItem::BSDFCollectionItem& BaseGroupItem::get_bsdf_collection_item() const
{
    return *m_bsdf_collection_item;
}

BaseGroupItem::BSSRDFCollectionItem& BaseGroupItem::get_bssrdf_collection_item() const
{
    return *m_bssrdf_collection_item;
}

BaseGroupItem::ColorCollectionItem& BaseGroupItem::get_color_collection_item() const
{
    return *m_color_collection_item;
}

BaseGroupItem::EDFCollectionItem& BaseGroupItem::get_edf_collection_item() const
{
    return *m_edf_collection_item;
}

BaseGroupItem::LightCollectionItem& BaseGroupItem::get_light_collection_item() const
{
    return *m_light_collection_item;
}

MaterialCollectionItem& BaseGroupItem::get_material_collection_item() const
{
    return *m_material_collection_item;
}

ObjectCollectionItem& BaseGroupItem::get_object_collection_item() const
{
    return *m_object_collection_item;
}

BaseGroupItem::ObjectInstanceCollectionItem& BaseGroupItem::get_object_instance_collection_item() const
{
    return *m_object_instance_collection_item;
}

BaseGroupItem::ShaderGroupCollectionItem& BaseGroupItem::get_shader_group_collection_item() const
{
    return *m_shader_group_collection_item;
}

BaseGroupItem::SurfaceShaderCollectionItem& BaseGroupItem::get_surface_shader_collection_item() const
{
    return *m_surface_shader_collection_item;
}

TextureCollectionItem& BaseGroupItem::get_texture_collection_item() const
{
    return *m_texture_collection_item;
}

BaseGroupItem::TextureInstanceCollectionItem& BaseGroupItem::get_texture_instance_collection_item() const
{
    return *m_texture_instance_collection_item;
}

BaseGroupItem::VolumeCollectionItem& BaseGroupItem::get_volume_collection_item() const
{
    return *m_volume_collection_item;
}

void BaseGroupItem::add_items(BaseGroup& base_group)
{
    addChild(
        m_color_collection_item =
            new ColorCollectionItem(
                m_editor_context,
                new_guid(),
                EntityTraits<ColorEntity>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_color_collection_item->add_items(base_group.colors());

    addChild(
        m_texture_collection_item =
            new TextureCollectionItem(
                m_editor_context,
                base_group.textures(),
                base_group,
                this));

    addChild(
        m_texture_instance_collection_item =
            new TextureInstanceCollectionItem(
                m_editor_context,
                new_guid(),
                EntityTraits<TextureInstance>::get_human_readable_collection_type_name(),
                base_group));
    m_texture_instance_collection_item->add_items(base_group.texture_instances());

    addChild(
        m_bsdf_collection_item =
            new MultiModelCollectionItem<BSDF, BaseGroup, BaseGroupItem>(
                m_editor_context,
                new_guid(),
                EntityTraits<BSDF>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_bsdf_collection_item->add_items(base_group.bsdfs());

    addChild(
        m_bssrdf_collection_item =
            new MultiModelCollectionItem<BSSRDF, BaseGroup, BaseGroupItem>(
                m_editor_context,
                new_guid(),
                EntityTraits<BSSRDF>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_bssrdf_collection_item->add_items(base_group.bssrdfs());

    addChild(
        m_edf_collection_item =
            new MultiModelCollectionItem<EDF, BaseGroup, BaseGroupItem>(
                m_editor_context,
                new_guid(),
                EntityTraits<EDF>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_edf_collection_item->add_items(base_group.edfs());

    addChild(
        m_surface_shader_collection_item =
            new MultiModelCollectionItem<SurfaceShader, BaseGroup, BaseGroupItem>(
                m_editor_context,
                new_guid(),
                EntityTraits<SurfaceShader>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_surface_shader_collection_item->add_items(base_group.surface_shaders());

    addChild(
        m_shader_group_collection_item =
            new ShaderGroupCollectionItem(
                m_editor_context,
                new_guid(),
                EntityTraits<ShaderGroup>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_shader_group_collection_item->add_items(base_group.shader_groups());

    addChild(
        m_material_collection_item =
            new MaterialCollectionItem(
                m_editor_context,
                base_group.materials(),
                base_group,
                this));

    addChild(
        m_light_collection_item =
            new MultiModelCollectionItem<Light, BaseGroup, BaseGroupItem>(
                m_editor_context,
                new_guid(),
                EntityTraits<Light>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_light_collection_item->add_items(base_group.lights());

    addChild(
        m_object_collection_item =
            new ObjectCollectionItem(
                m_editor_context,
                base_group.objects(),
                base_group,
                this));

    addChild(
        m_object_instance_collection_item =
            new ObjectInstanceCollectionItem(
                m_editor_context,
                new_guid(),
                EntityTraits<ObjectInstance>::get_human_readable_collection_type_name(),
                base_group));
    m_object_instance_collection_item->add_items(base_group.object_instances());

    addChild(
        m_volume_collection_item =
            new MultiModelCollectionItem<Volume, BaseGroup, BaseGroupItem>(
                m_editor_context,
                new_guid(),
                EntityTraits<Volume>::get_human_readable_collection_type_name(),
                base_group,
                this));
    m_volume_collection_item->add_items(base_group.volumes());

    addChild(
        m_assembly_collection_item =
            new AssemblyCollectionItem(
                m_editor_context,
                base_group.assemblies(),
                base_group,
                this));

    addChild(
        m_assembly_instance_collection_item =
            new AssemblyInstanceCollectionItem(
                m_editor_context,
                new_guid(),
                EntityTraits<AssemblyInstance>::get_human_readable_collection_type_name(),
                base_group));
    m_assembly_instance_collection_item->add_items(base_group.assembly_instances());
}

}   // namespace studio
}   // namespace appleseed
