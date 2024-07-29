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

// Returns the path to the `components/brave_ads/resources` directory, which
// contains bundled .pak data resources.
base::FilePath DataResourcesPath();

// Returns the path to the `components/brave_ads/core/test/data` directory,
// which contains all mocked data.
base::FilePath DataPath();

// Returns the path to the
// `components/brave_ads/core/test/data/resources/components` directory, which
// contains mocked resource components.
base::FilePath ResourceComponentsDataPath();

// Provides the path to the `components/brave_ads/core/test/data/url_responses`
// directory, which contains mocked URL responses for use with
// `MockUrlResponses`.
base::FilePath UrlResponsesDataPath();

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_FILE_PATH_TEST_UTIL_H_
