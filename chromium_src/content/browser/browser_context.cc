// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../../../../content/browser/browser_context.cc"  // NOLINT

namespace content {

bool BrowserContext::IsTorProfile() const {
  return false;
}

StoragePartition* BrowserContext::GetStoragePartitionForSite(
    BrowserContext* browser_context,
    const GURL& site,
    bool can_create) {
  return GetStoragePartitionForSite(browser_context, site, site, can_create);
}

}  // namespace content
