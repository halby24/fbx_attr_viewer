#include <fbxsdk.h>
#include <iostream>
#include <memory>
#include "DisplayCommon.h"

// Destroyメソッドを持つクラスかどうかをチェックするコンセプト
template <typename T>
concept HasDestroy = requires(T* t) {
    {
        t->Destroy()
    } -> std::same_as<void>;
};

// カスタムデリータ
template <typename T> struct FbxDeleter
{
    void operator()(T* p) const
    {
        if constexpr (HasDestroy<T>)
        {
            if (p) p->Destroy();
        } else
        {
            delete p;
        }
    }
};

// スマートポインタの型を定義
template <typename T> using FbxPtr = std::unique_ptr<T, FbxDeleter<T>>;

FbxPtr<FbxScene> import(const FbxPtr<FbxManager>& manager, const char* path);
void read(const FbxPtr<FbxScene>& scene);
void read_normal(FbxMesh* mesh);
void DisplayMaterial(FbxGeometry* pGeometry);
static const FbxImplementation* LookForImplementation(FbxSurfaceMaterial* pMaterial);

int main(int argc, char** argv)
{
    FbxPtr<FbxManager> manager(FbxManager::Create());
    if (manager.get() == nullptr)
    {
        std::cerr << "Error: Unable to create FBX Manager!" << std::endl;
        return 1;
    }

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input.fbx>" << std::endl;
        return 1;
    }

    const char* path = argv[1];
    auto scene = import(manager, path);
    if (scene == nullptr)
    {
        std::cerr << "Error: Unable to import FBX file!" << std::endl;
        return 1;
    }

    std::cout << "Imported FBX file: " << path << std::endl;

    read(scene);

    return 0;
}

void read(const FbxPtr<FbxScene>& scene)
{
    auto root = scene->GetRootNode();
    if (root == nullptr)
    {
        std::cerr << "Error: Root node is null!" << std::endl;
        return;
    }

    for (int i = 0; i < root->GetChildCount(); ++i)
    {
        auto child = root->GetChild(i);
        if (child == nullptr)
        {
            std::cerr << "Error: Child node is null!" << std::endl;
            continue;
        }

        std::cout << "Child node: " << child->GetName() << std::endl;

        auto attr = child->GetNodeAttribute();
        if (attr == nullptr)
        {
            std::cerr << "Error: Node attribute is null!" << std::endl;
            continue;
        }

        std::cout << "Node attribute: " << attr->GetAttributeType() << std::endl;

        if (attr->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            auto mesh = static_cast<FbxMesh*>(attr);
            std::cout << "Mesh: " << mesh->GetName() << std::endl;

            read_normal(mesh);
            DisplayMaterial(mesh);
        }
    }
}

FbxPtr<FbxScene> import(const FbxPtr<FbxManager>& manager, const char* path)
{
    if (manager == nullptr)
    {
        std::cerr << "Error: Unable to create FBX Manager!" << std::endl;
        return nullptr;
    }

    auto ios = FbxIOSettings::Create(manager.get(), IOSROOT);
    manager->SetIOSettings(ios);

    FbxPtr<FbxImporter> importer(FbxImporter::Create(manager.get(), ""));
    if (!importer->Initialize(path, -1, manager->GetIOSettings()))
    {
        std::cerr << "Error: Unable to initialize FBX importer!" << std::endl;
        return nullptr;
    }

    FbxPtr<FbxScene> scene(FbxScene::Create(manager.get(), ""));
    importer->Import(scene.get());

    return scene;
}

void read_normal(FbxMesh* mesh)
{
    auto elnrm = mesh->GetElementNormal();
    if (elnrm != nullptr)
    {
        std::cout << "Element normal: " << elnrm->GetName() << std::endl;

        // mapping mode is by control points. The mesh should be smooth and soft.
        // we can get normals by retrieving each control point
        if (elnrm->GetMappingMode() == FbxGeometryElement::eByControlPoint)
        {
            // Let's get normals of each vertex, since the mapping mode of normal element is by control point
            for (int vi = 0; vi < mesh->GetControlPointsCount(); vi++)
            {
                int ni = 0;
                // reference mode is direct, the normal index is same as vertex index.
                // get normals by the index of control vertex
                if (elnrm->GetReferenceMode() == FbxGeometryElement::eDirect) ni = vi;
                // reference mode is index-to-direct, get normals by the index-to-direct
                if (elnrm->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) ni = elnrm->GetIndexArray().GetAt(vi);
                // Got normals of each vertex.
                FbxVector4 normal = elnrm->GetDirectArray().GetAt(ni);
                // add your custom code here, to output normals or get them into a list, such as KArrayTemplate<FbxVector4>
                std::cout << "Normal for vertex " << vi << ": " << normal[0] << ", " << normal[1] << ", " << normal[2] << std::endl;
            } // end for lVertexIndex
        }     // end eByControlPoint
        // mapping mode is by polygon-vertex.
        // we can get normals by retrieving polygon-vertex.
        else if (elnrm->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        {
            int pvi = 0;
            // Let's get normals of each polygon, since the mapping mode of normal element is by polygon-vertex.
            for (int pi = 0; pi < mesh->GetPolygonCount(); pi++)
            {
                // get polygon size, you know how many vertices in current polygon.
                int ps = mesh->GetPolygonSize(pi);
                // retrieve each vertex of current polygon.
                for (int i = 0; i < ps; i++)
                {
                    int ni = 0;
                    // reference mode is direct, the normal index is same as lIndexByPolygonVertex.
                    if (elnrm->GetReferenceMode() == FbxGeometryElement::eDirect) ni = pvi;
                    // reference mode is index-to-direct, get normals by the index-to-direct
                    if (elnrm->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) ni = elnrm->GetIndexArray().GetAt(pvi);
                    // Got normals of each polygon-vertex.
                    FbxVector4 normal = elnrm->GetDirectArray().GetAt(ni);
                    // add your custom code here, to output normals or get them into a list, such as KArrayTemplate<FbxVector4>
                    std::cout << "Normal for polygon " << pi << " vertex " << i << ": " << normal[0] << ", " << normal[1] << ", " << normal[2] << std::endl;
                    pvi++;
                } // end for i //lPolygonSize
            }     // end for lPolygonIndex //PolygonCount
        }         // end eByPolygonVertex
    }
}

void DisplayMaterial(FbxGeometry* pGeometry)
{
    std::cout << "DisplayMaterial" << std::endl;

    int lMaterialCount = 0;
    FbxNode* lNode = NULL;
    if (pGeometry)
    {
        lNode = pGeometry->GetNode();
        std::cout << "Node: " << lNode->GetName() << std::endl;
        if (lNode) lMaterialCount = lNode->GetMaterialCount();
        std::cout << "Material count: " << lMaterialCount << std::endl;
    }
    if (lMaterialCount > 0)
    {
        FbxPropertyT<FbxDouble3> lKFbxDouble3;
        FbxPropertyT<FbxDouble> lKFbxDouble1;
        FbxColor theColor;
        for (int lCount = 0; lCount < lMaterialCount; lCount++)
        {
            DisplayInt("        Material ", lCount);
            FbxSurfaceMaterial* lMaterial = lNode->GetMaterial(lCount);
            DisplayString("            Name: \"", (char*)lMaterial->GetName(), "\"");
            // Get the implementation to see if it's a hardware shader.
            const FbxImplementation* lImplementation = LookForImplementation(lMaterial);
            if (lImplementation)
            {
                // Now we have a hardware shader, let's read it
                DisplayString("            Language: ", lImplementation->Language.Get().Buffer());
                DisplayString("            LanguageVersion: ", lImplementation->LanguageVersion.Get().Buffer());
                DisplayString("            RenderName: ", lImplementation->RenderName.Buffer());
                DisplayString("            RenderAPI: ", lImplementation->RenderAPI.Get().Buffer());
                DisplayString("            RenderAPIVersion: ", lImplementation->RenderAPIVersion.Get().Buffer());
                const FbxBindingTable* lRootTable = lImplementation->GetRootTable();
                FbxString lFileName = lRootTable->DescAbsoluteURL.Get();
                FbxString lTechniqueName = lRootTable->DescTAG.Get();
                const FbxBindingTable* lTable = lImplementation->GetRootTable();
                size_t lEntryNum = lTable->GetEntryCount();
                for (int i = 0; i < (int)lEntryNum; ++i)
                {
                    const FbxBindingTableEntry& lEntry = lTable->GetEntry(i);
                    const char* lEntrySrcType = lEntry.GetEntryType(true);
                    FbxProperty lFbxProp;
                    FbxString lTest = lEntry.GetSource();
                    DisplayString("            Entry: ", lTest.Buffer());
                    if (strcmp(FbxPropertyEntryView::sEntryType, lEntrySrcType) == 0)
                    {
                        lFbxProp = lMaterial->FindPropertyHierarchical(lEntry.GetSource());
                        if (!lFbxProp.IsValid()) { lFbxProp = lMaterial->RootProperty.FindHierarchical(lEntry.GetSource()); }
                    } else if (strcmp(FbxConstantEntryView::sEntryType, lEntrySrcType) == 0)
                    {
                        lFbxProp = lImplementation->GetConstants().FindHierarchical(lEntry.GetSource());
                    }
                    if (lFbxProp.IsValid())
                    {
                        if (lFbxProp.GetSrcObjectCount<FbxTexture>() > 0)
                        {
                            // do what you want with the textures
                            for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxFileTexture>(); ++j)
                            {
                                FbxFileTexture* lTex = lFbxProp.GetSrcObject<FbxFileTexture>(j);
                                DisplayString("           File Texture: ", lTex->GetFileName());
                            }
                            for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxLayeredTexture>(); ++j)
                            {
                                FbxLayeredTexture* lTex = lFbxProp.GetSrcObject<FbxLayeredTexture>(j);
                                DisplayString("        Layered Texture: ", lTex->GetName());
                            }
                            for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxProceduralTexture>(); ++j)
                            {
                                FbxProceduralTexture* lTex = lFbxProp.GetSrcObject<FbxProceduralTexture>(j);
                                DisplayString("     Procedural Texture: ", lTex->GetName());
                            }
                        } else
                        {
                            FbxDataType lFbxType = lFbxProp.GetPropertyDataType();
                            FbxString blah = lFbxType.GetName();
                            if (FbxBoolDT == lFbxType)
                            {
                                DisplayBool("                Bool: ", lFbxProp.Get<FbxBool>());
                            } else if (FbxIntDT == lFbxType || FbxEnumDT == lFbxType)
                            {
                                DisplayInt("                Int: ", lFbxProp.Get<FbxInt>());
                            } else if (FbxFloatDT == lFbxType)
                            {
                                DisplayDouble("                Float: ", lFbxProp.Get<FbxFloat>());
                            } else if (FbxDoubleDT == lFbxType)
                            {
                                DisplayDouble("                Double: ", lFbxProp.Get<FbxDouble>());
                            } else if (FbxStringDT == lFbxType || FbxUrlDT == lFbxType || FbxXRefUrlDT == lFbxType)
                            {
                                DisplayString("                String: ", lFbxProp.Get<FbxString>().Buffer());
                            } else if (FbxDouble2DT == lFbxType)
                            {
                                FbxDouble2 lDouble2 = lFbxProp.Get<FbxDouble2>();
                                FbxVector2 lVect;
                                lVect[0] = lDouble2[0];
                                lVect[1] = lDouble2[1];
                                Display2DVector("                2D vector: ", lVect);
                            } else if (FbxDouble3DT == lFbxType || FbxColor3DT == lFbxType)
                            {
                                FbxDouble3 lDouble3 = lFbxProp.Get<FbxDouble3>();
                                FbxVector4 lVect;
                                lVect[0] = lDouble3[0];
                                lVect[1] = lDouble3[1];
                                lVect[2] = lDouble3[2];
                                Display3DVector("                3D vector: ", lVect);
                            } else if (FbxDouble4DT == lFbxType || FbxColor4DT == lFbxType)
                            {
                                FbxDouble4 lDouble4 = lFbxProp.Get<FbxDouble4>();
                                FbxVector4 lVect;
                                lVect[0] = lDouble4[0];
                                lVect[1] = lDouble4[1];
                                lVect[2] = lDouble4[2];
                                lVect[3] = lDouble4[3];
                                Display4DVector("                4D vector: ", lVect);
                            } else if (FbxDouble4x4DT == lFbxType)
                            {
                                FbxDouble4x4 lDouble44 = lFbxProp.Get<FbxDouble4x4>();
                                for (int j = 0; j < 4; ++j)
                                {
                                    FbxVector4 lVect;
                                    lVect[0] = lDouble44[j][0];
                                    lVect[1] = lDouble44[j][1];
                                    lVect[2] = lDouble44[j][2];
                                    lVect[3] = lDouble44[j][3];
                                    Display4DVector("                4x4D vector: ", lVect);
                                }
                            }
                        }
                    }
                }
            } else if (lMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
            {
                // We found a Phong material.  Display its properties.
                // Display the Ambient Color
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Ambient;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Ambient: ", theColor);
                // Display the Diffuse Color
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Diffuse;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Diffuse: ", theColor);
                // Display the Specular Color (unique to Phong materials)
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Specular;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Specular: ", theColor);
                // Display the Emissive Color
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Emissive;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Emissive: ", theColor);
                // Opacity is Transparency factor now
                lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->TransparencyFactor;
                DisplayDouble("            Opacity: ", 1.0 - lKFbxDouble1.Get());
                // Display the Shininess
                lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->Shininess;
                DisplayDouble("            Shininess: ", lKFbxDouble1.Get());
                // Display the Reflectivity
                lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->ReflectionFactor;
                DisplayDouble("            Reflectivity: ", lKFbxDouble1.Get());
            } else if (lMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
            {
                // We found a Lambert material. Display its properties.
                // Display the Ambient Color
                lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Ambient;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Ambient: ", theColor);
                // Display the Diffuse Color
                lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Diffuse;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Diffuse: ", theColor);
                // Display the Emissive
                lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Emissive;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Emissive: ", theColor);
                // Display the Opacity
                lKFbxDouble1 = ((FbxSurfaceLambert*)lMaterial)->TransparencyFactor;
                DisplayDouble("            Opacity: ", 1.0 - lKFbxDouble1.Get());
            } else
                DisplayString("Unknown type of Material");
            FbxPropertyT<FbxString> lString;
            lString = lMaterial->ShadingModel;
            DisplayString("            Shading Model: ", lString.Get().Buffer());
            DisplayString("");
        }
    }
}

static const FbxImplementation* LookForImplementation(FbxSurfaceMaterial* pMaterial)
{
    const FbxImplementation* lImplementation = nullptr;
    if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_CGFX);
    if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_HLSL);
    if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_SFX);
    if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_OGS);
    if (!lImplementation) lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_SSSL);
    return lImplementation;
}
