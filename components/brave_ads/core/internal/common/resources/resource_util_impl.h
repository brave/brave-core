/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_IMPL_H_

#include <optional>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace base {
class File;
}  // namespace base

namespace brave_ads {

template <typename T>
std::optional<T> LoadAndParseResourceComponentOnBackgroundThread(
    base::File file) {
  if (!file.IsValid()) {
    return std::nullopt;
  }

  std::optional<base::Value::Dict> dict;
  {
    // `content` can be up to 10 MB, so we keep the scope of this object to this
    // block to release its memory as soon as possible.

    std::string content;
    const base::ScopedFILE scoped_file(base::FileToFILE(std::move(file), "rb"));
    if (!base::ReadStreamToString(&*scoped_file, &content)) {
      return std::nullopt;
    }

    dict = base::JSONReader::ReadDict(content);
    if (!dict) {
      return std::nullopt;
    }
  }

  return T::CreateFromValue(std::move(*dict));
}

template <typename T>
void LoadResourceComponentCallback(
    LoadAndParseResourceComponentCallback<T> callback,
    base::File file) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&LoadAndParseResourceComponentOnBackgroundThread<T>,
                     std::move(file)),
      std::move(callback));
}

template <typename T>
void LoadAndParseResourceComponent(
    const std::string& id,
    int version,
    LoadAndParseResourceComponentCallback<T> callback) {
  GetAdsClient().LoadResourceComponent(
      id, version,
      base::BindOnce(&LoadResourceComponentCallback<T>, std::move(callback)));
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_IMPL_H_
