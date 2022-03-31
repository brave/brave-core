/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_storage_cleanup.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "sql/database.h"

namespace {

constexpr const char* kDeprecatedAdsDataFiles[] = {
    "ad_conversions.json", "bundle.json", "catalog.json"};

constexpr const char* kDeprecatedBundleStateDb = "bundle_state";

}  // namespace

namespace brave_ads {

void RemoveDeprecatedAdsDataFiles(const base::FilePath& path) {
  for (const char* file : kDeprecatedAdsDataFiles) {
    base::DeleteFile(path.AppendASCII(file));
  }

  sql::Database::Delete(path.AppendASCII(kDeprecatedBundleStateDb));
}

}  // namespace brave_ads
