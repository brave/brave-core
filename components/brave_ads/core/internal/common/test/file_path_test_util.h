/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_FILE_PATH_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_FILE_PATH_TEST_UTIL_H_

namespace base {
class FilePath;
}  // namespace base

namespace brave_ads::test {

// Returns the path of `brave/components/brave_ads`.
base::FilePath RootPath();

// Returns the path of `brave/components/brave_ads/resources`.
base::FilePath DataResourcesPath();

// Returns the path of `brave/components/brave_ads/core/test/data`.
base::FilePath RootDataPath();

// Returns the path of
// `brave/components/brave_ads/core/test/data/component_resources`.
base::FilePath ComponentResourcesDataPath();

// Returns the path of
// `brave/components/brave_ads/core/test/data/url_responses`.
base::FilePath UrlResponsesDataPath();

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_FILE_PATH_TEST_UTIL_H_
