/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/containers/cookie_store_id.h"

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

namespace containers {
namespace {

constexpr char kStoreIdSeparator = ':';

// Matches chrome/browser/extensions/api/cookies/cookies_helpers.cc.
constexpr char kOriginalProfileStoreId[] = "0";
constexpr char kOffTheRecordProfileStoreId[] = "1";

}  // namespace

std::string GetContainerStoreId(std::string_view container_id) {
  CHECK(!container_id.empty());
  return base::StrCat({kContainersStoragePartitionDomain,
                       std::string_view(&kStoreIdSeparator, 1), container_id});
}

std::optional<std::string> ParseContainerStoreId(std::string_view store_id) {
  const std::string prefix =
      base::StrCat({kContainersStoragePartitionDomain,
                    std::string_view(&kStoreIdSeparator, 1)});
  if (!store_id.starts_with(prefix)) {
    return std::nullopt;
  }
  std::string_view container_id = store_id.substr(prefix.size());
  if (!IsValidStoragePartitionKeyComponent(container_id)) {
    return std::nullopt;
  }
  return std::string(container_id);
}

bool IsContainerStoreId(std::string_view store_id) {
  return ParseContainerStoreId(store_id).has_value();
}

std::string GetStoreIdForWebContents(content::WebContents* web_contents) {
  CHECK(web_contents);
  const std::string container_id = GetContainerIdForWebContents(web_contents);
  if (!container_id.empty()) {
    return GetContainerStoreId(container_id);
  }
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  return profile->IsOffTheRecord() ? kOffTheRecordProfileStoreId
                                   : kOriginalProfileStoreId;
}

}  // namespace containers
