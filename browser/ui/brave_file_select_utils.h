/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_FILE_SELECT_UTILS_H_
#define BRAVE_BROWSER_UI_BRAVE_FILE_SELECT_UTILS_H_

#include <string>

#include "base/containers/flat_map.h"

namespace content {
class WebContents;
}  // namespace content

namespace url {
class Origin;
}  // namespace url

namespace brave {

enum class SiteFrameTitleType {
  kStandardSameOrigin,
  kStandardDifferentOrigin,
  kNonStandardSameOrigin,
  kNonStandardDifferentOrigin
};

using SiteTitleResourceIDMap = base::flat_map<SiteFrameTitleType, int>;

std::u16string GetFileSelectTitle(content::WebContents* contents,
                                  const url::Origin& alerting_frame_origin,
                                  const SiteTitleResourceIDMap& resource_ids);

std::u16string GetSiteFrameTitleForFileSelect(
    SiteFrameTitleType type,
    const url::Origin& alerting_frame_origin,
    const SiteTitleResourceIDMap& resource_ids);

SiteFrameTitleType GetSiteFrameTitleType(
    const url::Origin& main_frame_origin,
    const url::Origin& alerting_frame_origin);

const SiteTitleResourceIDMap& GetFileSelectResourceIDsForOpen();
const SiteTitleResourceIDMap& GetFileSelectResourceIDsForSave();

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BRAVE_FILE_SELECT_UTILS_H_
