/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_NTP_SPONSORED_SOURCE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_NTP_SPONSORED_SOURCE_TEST_UTIL_H_

#include <string>

class GURL;

namespace base {
class FilePath;
}  // namespace base

namespace content {
class URLDataSource;
}  // namespace content

namespace ntp_background_images::test {

base::FilePath GetSponsoredImagesComponentPath();

std::string StartDataRequest(content::URLDataSource* url_data_source,
                             const GURL& url);

}  // namespace ntp_background_images::test

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_NTP_SPONSORED_SOURCE_TEST_UTIL_H_
