/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_REF_COUNTED_PROXY_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_REF_COUNTED_PROXY_BASE_H_

#include <string>
#include <utility>

#include "base/files/file.h"
#include "base/sequence_checker.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/core/internal/resources/resource_parsing_error_or.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

template <typename ResourceType>
class ResourceRefCounterProxyBase {
 public:
  ResourceRefCounterProxyBase() = default;
  virtual ~ResourceRefCounterProxyBase() = default;

  void AddConsumer();

  void RemoveConsumer();

  base::expected<bool, std::string> Load(base::File file,
                                         const std::string& manifest_version);

  const absl::optional<ResourceType>& GetResource() const;

 protected:
  SEQUENCE_CHECKER(sequence_checker_);

 private:
  void Reset();

  absl::optional<ResourceType> resource_;
  absl::optional<std::string> manifest_version_;

  int consumers_count_ = 0;
};

template <typename ResourceType>
void ResourceRefCounterProxyBase<ResourceType>::AddConsumer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++consumers_count_;
}

template <typename ResourceType>
void ResourceRefCounterProxyBase<ResourceType>::RemoveConsumer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  --consumers_count_;

  if (consumers_count_ == 0) {
    Reset();
  }
}

template <typename ResourceType>
base::expected<bool, std::string> ResourceRefCounterProxyBase<
    ResourceType>::Load(base::File file, const std::string& manifest_version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (manifest_version_ == manifest_version) {
    return base::ok(true);
  }

  ResourceParsingErrorOr<ResourceType> result =
      ReadFileAndParseResourceOnBackgroundThread<ResourceType>(std::move(file));
  if (!result.has_value()) {
    return base::unexpected(result.error());
  }

  if (!result.value().IsInitialized()) {
    return base::ok(false);
  }

  resource_ = std::move(result).value();
  manifest_version_ = manifest_version;

  return base::ok(true);
}

template <typename ResourceType>
const absl::optional<ResourceType>&
ResourceRefCounterProxyBase<ResourceType>::GetResource() const {
  return resource_;
}

template <typename ResourceType>
void ResourceRefCounterProxyBase<ResourceType>::Reset() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  resource_.reset();
  manifest_version_.reset();
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_REF_COUNTED_PROXY_BASE_H_
