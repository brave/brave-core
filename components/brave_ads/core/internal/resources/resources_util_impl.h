/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_RESOURCES_UTIL_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_RESOURCES_UTIL_IMPL_H_

#include "brave/components/brave_ads/core/internal/resources/resources_util.h"

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class File;
}  // namespace base

namespace brave_ads {

template <typename T>
base::expected<T, std::string> ReadFileAndParseResourceOnBackgroundThread(
    base::File file) {
  if (!file.IsValid()) {
    return base::unexpected("File is not valid");
  }

  absl::optional<base::Value> root;
  {
    std::string content;
    const base::ScopedFILE stream(base::FileToFILE(std::move(file), "rb"));
    if (!base::ReadStreamToString(stream.get(), &content)) {
      return base::unexpected("Couldn't read file");
    }

    root = base::JSONReader::Read(content);
    if (!root) {
      return base::unexpected("Failed to parse json");
    }
    // `content` can be up to 10 MB, so we keep the scope of this object to this
    // block to release its memory as soon as possible.
  }

  if (!root->is_dict()) {
    return base::unexpected("JSON is not a dictionary");
  }

  return T::CreateFromValue(std::move(root).value().TakeDict());
}

template <typename T>
void ReadFileAndParseResource(LoadAndParseResourceCallback<T> callback,
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
  AdsClientHelper::GetInstance()->LoadFileResource(
      id, version,
      base::BindOnce(&ReadFileAndParseResource<T>, std::move(callback)));
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_RESOURCES_UTIL_IMPL_H_
