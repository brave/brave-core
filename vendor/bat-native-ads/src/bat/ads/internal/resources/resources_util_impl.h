/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCES_UTIL_IMPL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCES_UTIL_IMPL_H_

#include "bat/ads/internal/resources/resources_util.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace resource {

template <typename T>
std::unique_ptr<ParsingResult<T>> ReadFileAndParseResourceOnBackgroundThread(
    base::File file) {
  if (!file.IsValid()) {
    return {};
  }
  std::string content;
  base::ScopedFILE stream(base::FileToFILE(std::move(file), "rb"));
  if (!base::ReadStreamToString(stream.get(), &content)) {
    return {};
  }

  absl::optional<base::Value> resource_value = base::JSONReader::Read(content);
  if (!resource_value) {
    return {};
  }

  // Free memory in advance to optimize the peak memory consumption. |json| can
  // be up to 10Mb and the following code allocates an extra memory few Mb of
  // memory.
  content = std::string();

  std::unique_ptr<ParsingResult<T>> result =
      std::make_unique<ParsingResult<T>>();
  result->resource =
      T::CreateFromValue(std::move(*resource_value), &result->error_message);

  return result;
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
  AdsClientHelper::Get()->LoadFileResource(
      id, version,
      base::BindOnce(&ReadFileAndParseResource<T>, std::move(callback)));
}

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCES_UTIL_IMPL_H_
