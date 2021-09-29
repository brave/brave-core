/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/page_info/chrome_page_info_ui_delegate.h"

#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"

#include "../../../../../../chrome/browser/ui/page_info/chrome_page_info_ui_delegate.cc"

bool ChromePageInfoUiDelegate::AddIPFSTabForURL(const GURL& ipfs_url) {
#if !defined(OS_ANDROID)
  chrome::AddTabAt(chrome::FindLastActiveWithProfile(GetProfile()),
                   GURL(ipfs_url), -1, true);
  return true;
#else
  return false;
#endif  // !defined(OS_ANDROID)
}
