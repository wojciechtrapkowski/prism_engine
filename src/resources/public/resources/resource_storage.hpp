#pragma once

#include "resources/resource.hpp"

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <optional>

namespace Prism::Resources {

    struct ResourceStorage {
        ResourceStorage() = default;
        ~ResourceStorage() = default;
        ResourceStorage(const ResourceStorage &) = delete;
        ResourceStorage &operator=(const ResourceStorage &) = delete;

        ResourceStorage(ResourceStorage &&) noexcept = default;
        ResourceStorage &operator=(ResourceStorage &&) noexcept = default;

        template <typename T>
            requires std::is_base_of<Resources::Resource, T>::value
        void Insert(Resource::ID id, std::unique_ptr<T> resource, size_t index = 0) {
            auto &storedResources = resources[id];
            if (index >= storedResources.size()) {
                storedResources.resize(index + 1);
            }
            storedResources[index] = std::move(resource);
        }

        void Delete(Resource::ID id, size_t index = 0) {
            auto &storedResources = resources[id];
            if (index < storedResources.size()) {
                storedResources.erase(storedResources.begin() + index);
            }
        }

        template <typename T>
            requires std::is_base_of<Resource, T>::value
        std::optional<std::reference_wrapper<T>> Get(Resource::ID id, size_t index = 0) const {
            auto it = resources.find(id);
            if (it == resources.end()) {
                return std::nullopt;
            }
            auto &storedResources = it->second;

            if (index < storedResources.size()) {
                if (storedResources[index] == nullptr) {
                    throw std::runtime_error("It shouldn't be that way - check that!");
                }
                return static_cast<T &>(*storedResources[index]);
            }

            return std::nullopt;
        }

        void Clear() { resources.clear(); }

      private:
        std::unordered_map<Resource::ID, std::vector<std::unique_ptr<Resources::Resource>>> resources = {};
    };

} // namespace Prism::Resources