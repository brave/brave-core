/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCES_UTIL_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCES_UTIL_IMPL_H_

#include "brave/components/brave_ads/core/internal/common/resources/resources_util.h"

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class File;
}  // namespace base

namespace brave_ads {

template <typename T>
base::expected<T, std::string> ReadFileAndParseResourceOnBackgroundThread(
    base::File file) {
  if (!file.IsValid()) {
    return base::ok(T{});
  }

  absl::optional<base::Value> root;
  {
    // |content| can be up to 10 MB, so we keep the scope of this object to this
    // block to release its memory as soon as possible.

    std::string content;
    const base::ScopedFILE scoped_file(base::FileToFILE(std::move(file), "rb"));
    if (!base::ReadStreamToString(scoped_file.get(), &content)) {
      return base::unexpected("Failed to read file");
    }

    root = base::JSONReader::Read(content);
    if (!root || !root->is_dict()) {
      return base::unexpected("Invalid JSON");
    }
  }

  return T::CreateFromValue(std::move(root).value().TakeDict());
}

template <typename T>
void LoadFileResourceCallback(LoadAndParseResourceCallback<T> callback,
                              base::File file) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileAndParseResourceOnBackgroundThread<T>,
                     std::move(file)),
      std::move(callback));
}

template <typename T>
void LoadAndParseResource(const std::string& id,
                          const int version,
                          LoadAndParseResourceCallback<T> callback) {
  LoadFileResource(
      id, version,
      base::BindOnce(&LoadFileResourceCallback<T>, std::move(callback)));
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCES_UTIL_IMPL_H_
