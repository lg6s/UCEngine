#pragma once

#include <uc_dev/gx/import/geo/multi_material_mesh.h>
#include <uc_dev/gx/import/fbx/fbx_common.h>
#include <uc_dev/gx/import/fbx/fbx_common_multi_material_mesh.h>

namespace uc
{
    namespace gx
    {
        namespace import
        {
            namespace fbx
            {
                //////////////////////
                inline std::shared_ptr<geo::multi_material_mesh> create_multi_material_mesh_internal(const fbxsdk::FbxMesh* mesh)
                {
                    const fbxsdk::FbxNode* mesh_node = mesh->GetNode();

                    //check
                    if (mesh == nullptr || !mesh->IsTriangleMesh())
                    {
                        ::uc::gx::throw_exception<uc::gx::fbx_exception>("file does not contain a triangle mesh");
                    }

                    assert(mesh_node);
                    assert(mesh->GetPolygonSize(0));

                    std::vector<geo::multi_material_mesh::positions_t> positions;   //positions used by every material
                    std::vector<geo::multi_material_mesh::uvs_t>       uvs;         //uvs used by every material

                    auto material_count = mesh->GetElementMaterialCount();

                    if (material_count == 0)
                    {
                        ::uc::gx::throw_exception<uc::gx::fbx_exception>("mesh does not have assigned materials");
                    }

                    std::vector< std::vector<int32_t> > materials_indices;

                    //assign polygons to materials
                    for (auto i = 0; i < material_count; ++i)
                    {
                        auto&& element_material = mesh->GetElementMaterial(i);
                        auto&& material_indices = element_material->GetIndexArray();
                        auto count = material_indices.GetCount();

                        if (element_material->GetMappingMode() == fbxsdk::FbxLayerElement::eAllSame)
                        {
                            auto polygon_count = mesh->GetPolygonCount();
                            auto material_index_for_polygon = material_indices.GetAt(0);

                            if (material_index_for_polygon + 1 > materials_indices.size())
                            {
                                materials_indices.resize(material_index_for_polygon + 1);
                            }

                            materials_indices[material_index_for_polygon].reserve(polygon_count);
                            for (auto k = 0; k < polygon_count; ++k)
                            {
                                materials_indices[material_index_for_polygon].push_back(k);
                            }
                        }
                        else
                        {
                            for (auto j = 0; j < count; ++j)
                            {
                                auto material_index_for_polygon = material_indices.GetAt(j);
                                if (material_index_for_polygon + 1 > materials_indices.size())
                                {
                                    materials_indices.resize(material_index_for_polygon + 1);
                                }

                                materials_indices[material_index_for_polygon].push_back(j);
                            }
                        }
                    }

                    //get_positions
                    positions.resize(materials_indices.size());
                    uvs.resize(materials_indices.size());

                    for (auto i = 0U; i < materials_indices.size(); ++i)
                    {
                        positions[i] = get_positions(mesh, materials_indices[i]);
                        uvs[i] = get_uvs(mesh, materials_indices[i]);
                    }

                    //reindex faces, these are indices in the separated positions and uvs
                    std::vector<geo::multi_material_mesh::faces_t>  faces; //uvs used by every material
                    faces.resize(materials_indices.size());

                    for (auto i = 0; i < faces.size(); ++i)
                    {
                        //reorient triangles ccw, since they come cw from fbx
                        for (auto j = 0; j < materials_indices[i].size(); ++j)
                        {
                            auto triangle = j;
                            geo::multi_material_mesh::face_t face;
                            face.v0 = triangle * 3;
                            face.v1 = triangle * 3 + 2;
                            face.v2 = triangle * 3 + 1;

                            faces[i].push_back(face);
                        }
                    }

                    return std::make_shared<geo::multi_material_mesh>(std::move(positions), std::move(uvs), std::move(faces), get_materials(mesh_node, static_cast<uint32_t>(materials_indices.size())));
                }

                //////////////////////
                inline std::shared_ptr<geo::multi_material_mesh> create_multi_material_mesh(const std::string& file_name)
                {
                    std::unique_ptr<fbxsdk::FbxManager, fbxmanager_deleter>     manager(fbxsdk::FbxManager::Create(), fbxmanager_deleter());
                    std::unique_ptr<fbxsdk::FbxScene, fbxscene_deleter>         scene(fbxsdk::FbxScene::Create(manager.get(), ""), fbxscene_deleter());
                    std::unique_ptr<fbxsdk::FbxImporter, fbximporter_deleter>   importer(fbxsdk::FbxImporter::Create(manager.get(), ""), fbximporter_deleter());

                    auto f = file_name;

                    auto import_status = importer->Initialize(f.c_str(), -1, manager->GetIOSettings());

                    if (import_status == false)
                    {
                        auto status = importer->GetStatus();
                        auto error = status.GetErrorString();
                        ::uc::gx::throw_exception<uc::gx::fbx_exception>(error);
                    }

                    import_status = importer->Import(scene.get());
                    if (import_status == false)
                    {
                        auto status = importer->GetStatus();
                        auto error = status.GetErrorString();
                        ::uc::gx::throw_exception<uc::gx::fbx_exception>(error);
                    }

                    FbxGeometryConverter geometryConverter(manager.get());
                    geometryConverter.Triangulate(scene.get(), true);

                    FbxAxisSystem scene_axis_system = scene->GetGlobalSettings().GetAxisSystem();
                    FbxAxisSystem our_axis_system = FbxAxisSystem(FbxAxisSystem::EPreDefinedAxisSystem::eDirectX);

                    if (scene_axis_system != our_axis_system)
                    {
                        our_axis_system.ConvertScene(scene.get());
                    }

                    FbxSystemUnit units = scene->GetGlobalSettings().GetSystemUnit();
                    FbxSystemUnit meters = FbxSystemUnit::m;

                    if (units != FbxSystemUnit::m)
                    {
                        //FbxSystemUnit::m.ConvertScene(scene.get());
                    }

                    std::vector<fbxsdk::FbxMesh*> meshes;
                    meshes = get_meshes(scene->GetRootNode(), meshes);

                    for (auto& m : meshes)
                    {
                        m->RemoveBadPolygons();
                        m->ComputeBBox();
                    }

                    std::vector<  std::shared_ptr<geo::multi_material_mesh> > multimeshes;
                    for (auto& m : meshes)
                    {
                        multimeshes.push_back(create_multi_material_mesh_internal(m));
                    }

                    //merge all multimaterial meshes into one
                    std::vector< geo::multi_material_mesh::positions_t > pos;
                    std::vector< geo::multi_material_mesh::uvs_t >       uv;
                    std::vector< geo::multi_material_mesh::faces_t >     faces;
                    std::vector< geo::multi_material_mesh::material >    mat;

                    for (auto&& m : multimeshes)
                    {
                        pos.insert(pos.end(), m->m_positions.begin(), m->m_positions.end());
                        uv.insert(uv.end(), m->m_uv.begin(), m->m_uv.end());
                        faces.insert(faces.end(), m->m_faces.begin(), m->m_faces.end());
                        mat.insert(mat.end(), m->m_materials.begin(), m->m_materials.end());
                        
                    }

                    return std::make_shared<geo::multi_material_mesh>(std::move(pos), std::move(uv), std::move(faces), std::move(mat));
                }
            }
        }
    }
}
