/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/page_info/chrome_page_info_delegate.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/page_info/page_info.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/tab_dialogs.h"

// Implemented in //brave/browser/ui/peek:page_info.
namespace peek {
void ShowCookiesDialogInTab(content::WebContents* web_contents);
}  // namespace peek
#endif

#define OpenCookiesDialog OpenCookiesDialog_ChromiumImpl
#include <chrome/browser/ui/page_info/chrome_page_info_delegate.cc>
#undef OpenCookiesDialog

bool ChromePageInfoDelegate::BraveShouldShowPermission(
    ContentSettingsType type) {
  if ((content_settings::IsShieldsContentSettingsType(type) ||
       type == ContentSettingsType::GEOLOCATION) &&
      GetProfile()->IsTor()) {
    return false;
  }
  return true;
}

#if !BUILDFLAG(IS_ANDROID)
void ChromePageInfoDelegate::OpenCookiesDialog() {
  // Only tabs have `TabDialogs`. Page info can also be shown for contents that
  // aren't tabs, e.g. Peek, which shows the dialog after moving into a tab.
  if (!TabDialogs::FromWebContents(web_contents_)) {
    peek::ShowCookiesDialogInTab(web_contents_);
    return;
  }
  OpenCookiesDialog_ChromiumImpl();
}
#endif
