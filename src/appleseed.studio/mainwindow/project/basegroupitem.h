
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
#include "mainwindow/project/itembase.h"

// appleseed.foundation headers.
#include "foundation/utility/uid.h"

// Forward declarations.
namespace appleseed { namespace studio { class AssemblyCollectionItem; } }
namespace appleseed { namespace studio { class AssemblyInstanceItem; } }
namespace appleseed { namespace studio { class EntityEditorContext; } }
namespace appleseed { namespace studio { template <typename Entity, typename EntityItem, typename ParentEntity> class InstanceCollectionItem; } }
namespace appleseed { namespace studio { template <typename Entity, typename EntityItem, typename ParentEntity> class InstanceCollectionItem; } }
namespace appleseed { namespace studio { template <typename Entity, typename ParentEntity, typename ParentItem> class CollectionItem; } }
namespace appleseed { namespace studio { template <typename Entity, typename ParentEntity, typename ParentItem> class CollectionItem; } }
namespace appleseed { namespace studio { template <typename Entity, typename ParentEntity, typename ParentItem> class SingleModelCollectionItem; } }
namespace appleseed { namespace studio { class ItemBase; } }
namespace appleseed { namespace studio { class MaterialCollectionItem; } }
namespace appleseed { namespace studio { class ObjectCollectionItem; } }
namespace appleseed { namespace studio { class ObjectInstanceItem; } }
namespace appleseed { namespace studio { class TextureCollectionItem; } }
namespace appleseed { namespace studio { class TextureInstanceItem; } }
namespace renderer  { class Assembly; }
namespace renderer  { class AssemblyInstance; }
namespace renderer  { class BSDF; }
namespace renderer  { class BSSRDF; }
namespace renderer  { class BaseGroup; }
namespace renderer  { class ColorEntity; }
namespace renderer  { class EDF; }
namespace renderer  { class Light; }
namespace renderer  { class Material; }
namespace renderer  { class Object; }
namespace renderer  { class ObjectInstance; }
namespace renderer  { class ShaderGroup; }
namespace renderer  { class SurfaceShader; }
namespace renderer  { class Texture; }
namespace renderer  { class TextureInstance; }
namespace renderer  { class Volume; }
class QMenu;
class QString;

namespace appleseed {
namespace studio {

class BaseGroupItem
  : public ItemBase
{
  public:
    BaseGroupItem(
        EntityEditorContext&            editor_context,
        const foundation::UniqueID      class_uid,
        renderer::BaseGroup&            base_group);

    BaseGroupItem(
        EntityEditorContext&            editor_context,
        const foundation::UniqueID      class_uid,
        const QString&                  title,
        renderer::BaseGroup&            base_group);

    ItemBase* add_item(renderer::Assembly* assembly);
    ItemBase* add_item(renderer::AssemblyInstance* assembly_instance);
    ItemBase* add_item(renderer::BSDF* bsdf);
    ItemBase* add_item(renderer::BSSRDF* bssrdf);
    ItemBase* add_item(renderer::ColorEntity* color);
    ItemBase* add_item(renderer::EDF* edf);
    ItemBase* add_item(renderer::Light* light);
    ItemBase* add_item(renderer::Material* material);
    ItemBase* add_item(renderer::Object* object);
    ItemBase* add_item(renderer::ObjectInstance* object_instance);
    ItemBase* add_item(renderer::ShaderGroup* shader_group);
    ItemBase* add_item(renderer::SurfaceShader* surface_shader);
    ItemBase* add_item(renderer::Texture* texture);
    ItemBase* add_item(renderer::TextureInstance* texture_instance);
    ItemBase* add_item(renderer::Volume* volume);

    typedef CollectionItem<renderer::BSDF, renderer::BaseGroup, BaseGroupItem> BSDFCollectionItem;
    typedef SingleModelCollectionItem<renderer::ColorEntity, renderer::BaseGroup, BaseGroupItem> ColorCollectionItem;
    typedef InstanceCollectionItem<renderer::AssemblyInstance, AssemblyInstanceItem, renderer::BaseGroup> AssemblyInstanceCollectionItem;
    typedef InstanceCollectionItem<renderer::TextureInstance, TextureInstanceItem, renderer::BaseGroup> TextureInstanceCollectionItem;
    typedef SingleModelCollectionItem<renderer::ShaderGroup, renderer::BaseGroup, BaseGroupItem> ShaderGroupCollectionItem;
    typedef CollectionItem<renderer::BSSRDF, renderer::BaseGroup, BaseGroupItem> BSSRDFCollectionItem;
    typedef CollectionItem<renderer::EDF, renderer::BaseGroup, BaseGroupItem> EDFCollectionItem;
    typedef CollectionItem<renderer::SurfaceShader, renderer::BaseGroup, BaseGroupItem> SurfaceShaderCollectionItem;
    typedef CollectionItem<renderer::Light, renderer::BaseGroup, BaseGroupItem> LightCollectionItem;
    typedef CollectionItem<renderer::Volume, renderer::BaseGroup, BaseGroupItem> VolumeCollectionItem;

    typedef InstanceCollectionItem<renderer::ObjectInstance, ObjectInstanceItem, renderer::BaseGroup> ObjectInstanceCollectionItem;

    AssemblyCollectionItem& get_assembly_collection_item() const;
    AssemblyInstanceCollectionItem& get_assembly_instance_collection_item() const;
    BSDFCollectionItem& get_bsdf_collection_item() const;
    BSSRDFCollectionItem& get_bssrdf_collection_item() const;
    ColorCollectionItem& get_color_collection_item() const;
    EDFCollectionItem& get_edf_collection_item() const;
    LightCollectionItem& get_light_collection_item() const;
    MaterialCollectionItem& get_material_collection_item() const;
    ObjectCollectionItem& get_object_collection_item() const;
    ObjectInstanceCollectionItem& get_object_instance_collection_item() const;
    ShaderGroupCollectionItem& get_shader_group_collection_item() const;
    SurfaceShaderCollectionItem& get_surface_shader_collection_item() const;
    TextureCollectionItem& get_texture_collection_item() const;
    TextureInstanceCollectionItem& get_texture_instance_collection_item() const;
    VolumeCollectionItem& get_volume_collection_item() const;

  private:
    AssemblyCollectionItem*         m_assembly_collection_item;
    AssemblyInstanceCollectionItem* m_assembly_instance_collection_item;
    BSDFCollectionItem*             m_bsdf_collection_item;
    BSSRDFCollectionItem*           m_bssrdf_collection_item;
    ColorCollectionItem*            m_color_collection_item;
    EDFCollectionItem*              m_edf_collection_item;
    LightCollectionItem*            m_light_collection_item;
    MaterialCollectionItem*         m_material_collection_item;
    ObjectCollectionItem*           m_object_collection_item;
    ObjectInstanceCollectionItem*   m_object_instance_collection_item;
    ShaderGroupCollectionItem*      m_shader_group_collection_item;
    SurfaceShaderCollectionItem*    m_surface_shader_collection_item;
    TextureCollectionItem*          m_texture_collection_item;
    TextureInstanceCollectionItem*  m_texture_instance_collection_item;
    VolumeCollectionItem*           m_volume_collection_item;

    void add_items(renderer::BaseGroup& base_group);
};

}   // namespace studio
}   // namespace appleseed
