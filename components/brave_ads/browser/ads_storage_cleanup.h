/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_STORAGE_CLEANUP_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_STORAGE_CLEANUP_H_

namespace base {
class FilePath;
}

namespace brave_ads {

// Remove deprecated ads data files from ads_service directory.
void RemoveDeprecatedAdsDataFiles(const base::FilePath& path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_STORAGE_CLEANUP_H_
