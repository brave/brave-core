/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_IMPL_H_

#include <optional>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

// Not templated on `T`; reading and parsing the file does not depend on
// which resource type is loading it.
std::optional<base::DictValue> ReadResourceComponentFileToDict(base::File file);

template <typename T>
std::optional<T> LoadAndParseResourceComponentOnBackgroundThread(
    base::File file) {
  std::optional<base::DictValue> dict =
      ReadResourceComponentFileToDict(std::move(file));
  if (!dict) {
    return std::nullopt;
  }

  return T::MaybeFromDict(std::move(*dict));
}

template <typename T>
void OnLoadResourceComponent(
    LoadAndParseResourceComponentCallback<T> callback,
    base::File file,
    bool exists) {
  if (!exists) {
    return std::move(callback).Run(/*resource=*/std::nullopt,
                                   /*exists=*/false);
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&LoadAndParseResourceComponentOnBackgroundThread<T>,
                     std::move(file)),
      base::BindOnce(
          [](LoadAndParseResourceComponentCallback<T> callback,
             std::optional<T> resource) {
            std::move(callback).Run(std::move(resource),
                                    /*exists=*/true);
          },
          std::move(callback)));
}

template <typename T>
void LoadAndParseResourceComponent(
    const std::string& id,
    int version,
    LoadAndParseResourceComponentCallback<T> callback) {
  GetAdsClient().LoadResourceComponent(
      id, version,
      base::BindOnce(&OnLoadResourceComponent<T>, std::move(callback)));
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_IMPL_H_
