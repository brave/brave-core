/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/resources_util.h"

#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"
#include "bat/ads/internal/resources/conversions/conversions_info.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace resource {

namespace {

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

}  // namespace

template <typename T>
void LoadAndParseResource(const std::string& id,
                          const int version,
                          LoadAndParseResourceCallback<T> callback) {
  AdsClientHelper::Get()->LoadFileResource(
      id, version,
      base::BindOnce(&ReadFileAndParseResource<T>, std::move(callback)));
}

// Explicit instantiation of function for ml::pipeline::TextProcessing.
template void LoadAndParseResource<ml::pipeline::TextProcessing>(
    const std::string& id,
    const int version,
    LoadAndParseResourceCallback<ml::pipeline::TextProcessing> callback);

// Explicit instantiation of function for ad_targeting::PurchaseIntentInfo.
template void LoadAndParseResource<ad_targeting::PurchaseIntentInfo>(
    const std::string& id,
    const int version,
    LoadAndParseResourceCallback<ad_targeting::PurchaseIntentInfo> callback);

// Explicit instantiation of function for ConversionsInfo.
template void LoadAndParseResource<ConversionsInfo>(
    const std::string& id,
    const int version,
    LoadAndParseResourceCallback<ConversionsInfo> callback);

// Explicit instantiation of function for AntiTargetingInfo.
template void LoadAndParseResource<AntiTargetingInfo>(
    const std::string& id,
    const int version,
    LoadAndParseResourceCallback<AntiTargetingInfo> callback);

}  // namespace resource
}  // namespace ads
