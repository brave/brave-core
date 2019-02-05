/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_UTIL_H_

#include <memory>

#include "base/files/file_path.h"

class KeyedService;
class Profile;

namespace brave_ads {

std::unique_ptr<Profile> CreateBraveAdsProfile(const base::FilePath& path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_UTIL_H_
