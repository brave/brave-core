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

// This enum values are used to determine the title of the file select dialog.
// Basically it mirrors std::u16string
// AppModalDialogManager::GetSiteFrameTitle() implementation.
enum class SiteFrameTitleType {
  kStandardSameOrigin,  // alerting frame has http(s) scheme and has the same
                        // origin with main frame
  kStandardDifferentOrigin,  // alerting frame http(s) scheme and has a
                             // different origin with main frame
  kNonStandardSameOrigin,    // alerting frame has other schemes (e.g. file,
                             // data, javascript) and has the same origin with
                             // main frame
  kNonStandardDifferentOrigin,  // alerting frame has other schemes (e.g. file,
                                // data, javascript) and has a different origin
                                // with main frame
  kSize
};

enum class FileSelectTitleType {
  kOpen,
  kSave,
  kChromiumDefault,  // used for comparing with the default title of the file
                     // select dialog in Chromium
  kSize
};

std::u16string GetFileSelectTitle(content::WebContents* contents,
                                  const url::Origin& alerting_frame_origin,
                                  const url::Origin& download_origin,
                                  FileSelectTitleType file_select_type);

std::u16string GetSiteFrameTitleForFileSelect(
    SiteFrameTitleType frame_type,
    const url::Origin& alerting_frame_origin,
    FileSelectTitleType select_type);

SiteFrameTitleType GetSiteFrameTitleType(
    const url::Origin& main_frame_origin,
    const url::Origin& alerting_frame_origin);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BRAVE_FILE_SELECT_UTILS_H_
