/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/site_instance_impl.h"

#include <content/browser/site_instance_impl.cc>

namespace content {

// Used by the containers feature so that SiteInstance::GetSiteInstanceForNewTab
// can apply a container's StoragePartitionConfig when opening a new tab page,
// rather than always falling back to the default partition via CreateForURL.
scoped_refptr<SiteInstance>
SiteInstance::CreateForURLWithOptionalFixedStoragePartition(
    BrowserContext* browser_context,
    const GURL& url,
    base::optional_ref<StoragePartitionConfig> storage_partition_config) {
  if (storage_partition_config) {
    return CreateForFixedStoragePartition(browser_context, url,
                                          *storage_partition_config);
  } else {
    return CreateForURL(browser_context, url);
  }
}

}  // namespace content
