#include <fbxsdk.h>
#include <iostream>
#include <memory>

// Destroyメソッドを持つクラスかどうかをチェックするコンセプト
template<typename T>
concept HasDestroy = requires(T* t) {
    { t->Destroy() } -> std::same_as<void>;
};

// カスタムデリータ
template<typename T>
struct FbxDeleter {
    void operator()(T* p) const {
        if constexpr (HasDestroy<T>) {
            if (p) p->Destroy();
        } else {
            delete p;
        }
    }
};

// スマートポインタの型を定義
template<typename T>
using FbxPtr = std::unique_ptr<T, FbxDeleter<T>>;

FbxPtr<FbxScene> import(const FbxPtr<FbxManager>& manager, const char* path);

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

    

    return 0;
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