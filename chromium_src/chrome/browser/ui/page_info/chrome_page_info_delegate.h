/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_INFO_CHROME_PAGE_INFO_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_INFO_CHROME_PAGE_INFO_DELEGATE_H_

#include "components/page_info/page_info_delegate.h"

#define IsContentDisplayedInVrHeadset                           \
  BraveShouldShowPermission(ContentSettingsType type) override; \
  virtual bool IsContentDisplayedInVrHeadset

#include "src/chrome/browser/ui/page_info/chrome_page_info_delegate.h"
#undef IsContentDisplayedInVrHeadset

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_INFO_CHROME_PAGE_INFO_DELEGATE_H_
