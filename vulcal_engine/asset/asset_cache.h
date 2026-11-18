#pragma once

#include "asset/asset_id.h"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace vulcal::asset {

template<typename T, typename ID>
class AssetCache {
public:
    using Pointer = std::shared_ptr<const T>;

    Pointer find(ID id) const {
        const auto it = assets_.find(id);
        return it == assets_.end() ? Pointer{} : it->second;
    }

    bool failed(ID id) const { return failed_.contains(id); }

    void store(ID id, Pointer asset) {
        assets_.insert_or_assign(id, std::move(asset));
        failed_.erase(id);
    }

    void mark_failed(ID id) { failed_.insert(id); }

    bool erase(ID id) {
        const bool cached = assets_.erase(id) != 0;
        const bool failed = failed_.erase(id) != 0;
        return cached || failed;
    }

    std::size_t size() const { return assets_.size(); }
    std::size_t failure_count() const { return failed_.size(); }
    bool empty() const { return assets_.empty() && failed_.empty(); }

    void clear() {
        assets_.clear();
        failed_.clear();
    }

private:
    std::unordered_map<ID, Pointer> assets_;
    std::unordered_set<ID> failed_;
};

} // namespace vulcal::asset
