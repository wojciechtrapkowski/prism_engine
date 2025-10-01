#pragma once

#include "resources/resource.hpp"

#include "resources/mesh_resource.hpp"

#include <entt/entt.hpp>


#include <optional>
#include <utility>

namespace Prism::Resources {
    struct Scene : ResourceImpl<Scene> {
        Scene() = default;
        ~Scene() = default;

        Scene &operator=(Scene &&other) = default;
        Scene(Scene &&other) = default;

        // For now, will be implemented in the future. Scene should be copyable.
        Scene(Scene &other) = delete;
        Scene &operator=(Scene &other) = delete;

        entt::registry &GetRegistry() { return m_registry; }

        std::optional<std::reference_wrapper<MeshResource>> GetMesh(Resources::MeshResource::ID resourceId);

        void AddNewMesh(Resources::MeshResource::ID id, std::string name, std::unique_ptr<Resources::MeshResource> meshResource);

        void RemoveMesh(Resources::MeshResource::ID meshId);

      private:
        entt::registry m_registry;

        std::unordered_map<Resources::MeshResource::ID, std::unique_ptr<Resources::MeshResource>> m_meshes;
    };
}; // namespace Prism::Resources