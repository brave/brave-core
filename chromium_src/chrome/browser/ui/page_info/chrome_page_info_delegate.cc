/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/page_info/chrome_page_info_delegate.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"

namespace chrome {
std::string GetIPFSLearnMoreURL(const GURL& url) {
  if (ipfs::IsIPFSScheme(url))
    return ipfs::kIPFSLearnMoreURL;
  return std::string(chrome::kPageInfoHelpCenterURL);
}
}  // namespace chrome

#define kPageInfoHelpCenterURL GetIPFSLearnMoreURL(web_contents_->GetURL())
#endif  // BUILDFLAG(IPFS_ENABLED)

#include "../../../../../../chrome/browser/ui/page_info/chrome_page_info_delegate.cc"
#if BUILDFLAG(IPFS_ENABLED)
#undef kPageInfoHelpCenterURL
#endif  // BUILDFLAG(IPFS_ENABLED)

bool ChromePageInfoDelegate::BraveShouldShowPermission(
    ContentSettingsType type) {
  if ((content_settings::IsShieldsContentSettingsType(type) ||
       type == ContentSettingsType::GEOLOCATION) &&
      GetProfile()->IsTor()) {
    return false;
  }
  return true;
}
