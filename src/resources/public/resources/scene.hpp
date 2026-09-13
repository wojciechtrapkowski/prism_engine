#pragma once

#include "resources/resource.hpp"
#include "resources/resource_storage.hpp"

#include "resources/mesh_resource.hpp"

#include <entt/entt.hpp>

#include <optional>
#include <utility>

namespace Prism::Resources
{
    struct Scene : ResourceImpl<Scene>
    {
        Scene()  = default;
        ~Scene() = default;

        Scene& operator=(Scene&& other) = default;
        Scene(Scene&& other)            = default;

        // For now, will be implemented in the future. Scene should be copyable.
        Scene(Scene& other)            = delete;
        Scene& operator=(Scene& other) = delete;

        entt::registry&             GetRegistry() { return _registry; }
        Resources::ResourceStorage& GetMeshStorage() { return _meshStorage; }
        Resources::ResourceStorage& GetSystemsStorage() { return _systemsStorage; }

    private:
        entt::registry _registry;

        Resources::ResourceStorage _meshStorage;
        Resources::ResourceStorage _systemsStorage;
    };
}; // namespace Prism::Resources