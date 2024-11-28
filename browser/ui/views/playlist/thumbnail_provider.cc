/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/thumbnail_provider.h"

#include <memory>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/containers/lru_cache.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "net/base/filename_util.h"

namespace {

using ItemImageCache = base::LRUCache<std::string /*item_id*/, gfx::Image>;

ItemImageCache& GetInMemoryCache(playlist::PlaylistService* service) {
  auto key = reinterpret_cast<uintptr_t>(service);

  // We don't want to mix up images from different services, as the
  // PlaylistService is bound to a Profile.
  static base::NoDestructor<base::flat_map<uintptr_t, ItemImageCache>> s_cache;
  auto it = s_cache->find(key);
  if (it == s_cache->end()) {
    it = s_cache->insert({key, ItemImageCache(/*max size=*/30)}).first;
  }
  return it->second;
}

bool IsItemThumbnailCached(const playlist::mojom::PlaylistItemPtr& item) {
  return item->thumbnail_path.is_valid() &&
         item->thumbnail_path != item->thumbnail_source;
}

}  // namespace

ThumbnailProvider::ThumbnailProvider(playlist::PlaylistService& service)
    : service_(service) {}

ThumbnailProvider::ThumbnailProvider(playlist::PlaylistTabHelper* tab_helper)
    : ThumbnailProvider(*playlist::PlaylistServiceFactory::GetForBrowserContext(
          tab_helper->web_contents()->GetBrowserContext())) {}

ThumbnailProvider::~ThumbnailProvider() = default;

void ThumbnailProvider::GetThumbnail(
    const playlist::mojom::PlaylistItemPtr& item,
    base::OnceCallback<void(const gfx::Image&)> callback) {
  DVLOG(2) << __FUNCTION__;
  if (IsItemThumbnailCached(item)) {
    if (base::FilePath thumbnail_path;
        net::FileURLToFilePath(item->thumbnail_path, &thumbnail_path)) {
      base::ThreadPool::PostTaskAndReplyWithResult(
          FROM_HERE, base::MayBlock(),
          base::BindOnce(
              [](base::FilePath path) {
                std::string raw_data;
                if (base::ReadFileToString(path, &raw_data)) {
                  return gfx::Image::CreateFrom1xPNGBytes(
                      base::as_byte_span(raw_data));
                }

                VLOG(2) << __FUNCTION__ << " Failed to read " << path;
                return gfx::Image();
              },
              thumbnail_path),
          base::BindOnce(&ThumbnailProvider::OnGotThumbnail,
                         weak_ptr_factory_.GetWeakPtr(), item->id,
                         /*from_network=*/false, std::move(callback)));
      return;
    }
  }

  if (!item->thumbnail_source.is_valid()) {
    std::move(callback).Run({});
    return;
  }

  auto& in_memory_cache = GetInMemoryCache(base::to_address(service_));
  if (auto iter = in_memory_cache.Get(item->id);
      iter != in_memory_cache.end()) {
    std::move(callback).Run(iter->second);
    return;
  }

  service_->DownloadThumbnail(
      item->thumbnail_source,
      base::BindOnce(&ThumbnailProvider::OnGotThumbnail,
                     weak_ptr_factory_.GetWeakPtr(), item->id,
                     /*from_network=*/true, std::move(callback)));
}

void ThumbnailProvider::GetThumbnail(
    const playlist::mojom::PlaylistPtr& list,
    base::OnceCallback<void(const gfx::Image&)> callback) {
  DVLOG(2) << __FUNCTION__;
  if (!list->id.has_value()) {
    std::move(callback).Run({});
    return;
  }

  if (list->id == playlist::kDefaultPlaylistID) {
    // If list is default folder, return default thumbnail image
    // TODO(sko) We need to set default player folder icon soon.
    std::move(callback).Run({});
    return;
  }

  // Else, find the first cached thumbnail.
  auto iter = base::ranges::find_if(list->items, &IsItemThumbnailCached);
  if (iter == list->items.end()) {
    std::move(callback).Run({});
    return;
  }

  GetThumbnail(iter->Clone(), std::move(callback));
}

void ThumbnailProvider::OnGotThumbnail(
    const std::string& id,
    bool from_network,
    base::OnceCallback<void(const gfx::Image&)> callback,
    gfx::Image thumbnail) {
  if (!thumbnail.IsEmpty()) {
    DCHECK(!id.empty());
    auto& in_memory_cache = GetInMemoryCache(base::to_address(service_));
    if (from_network) {
      in_memory_cache.Put({id, thumbnail});
    } else if (in_memory_cache.Peek(id) != in_memory_cache.end()) {
      in_memory_cache.Erase(in_memory_cache.Get(id));
    }
  }

  std::move(callback).Run(thumbnail);
}
