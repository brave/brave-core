/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_FLATBUFFERS_RESOURCES_UTIL_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_FLATBUFFERS_RESOURCES_UTIL_IMPL_H_

#include "brave/components/brave_ads/core/internal/common/resources/flatbuffers_resources_util.h"

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"

namespace brave_ads {

template <typename T>
base::expected<T, std::string> ReadFlatBuffersResourceOnBackgroundThread(
    base::File file) {
  if (!file.IsValid()) {
    return base::ok(T{});
  }

  std::string buffer;
  const base::ScopedFILE scoped_file(base::FileToFILE(std::move(file), "rb"));
  if (!base::ReadStreamToString(scoped_file.get(), &buffer)) {
    return base::unexpected("Failed to read file");
  }

  return T::CreateFromFlatBuffers(std::move(buffer));
}

template <typename T>
void LoadFileFlatBuffersResourceCallback(
    LoadFlatBuffersResourceCallback<T> callback,
    base::File file) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFlatBuffersResourceOnBackgroundThread<T>,
                     std::move(file)),
      std::move(callback));
}

template <typename T>
void LoadFlatBuffersResource(const std::string& id,
                             const int version,
                             LoadFlatBuffersResourceCallback<T> callback) {
  LoadFileResource(id, version,
                   base::BindOnce(&LoadFileFlatBuffersResourceCallback<T>,
                                  std::move(callback)));
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_FLATBUFFERS_RESOURCES_UTIL_IMPL_H_
