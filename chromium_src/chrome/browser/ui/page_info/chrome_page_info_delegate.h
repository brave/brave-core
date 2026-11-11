/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_INFO_CHROME_PAGE_INFO_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_INFO_CHROME_PAGE_INFO_DELEGATE_H_

#include "components/page_info/page_info_delegate.h"

#define OpenCookiesDialog           \
  OpenCookiesDialog_ChromiumImpl(); \
  void OpenCookiesDialog

#include <chrome/browser/ui/page_info/chrome_page_info_delegate.h>  // IWYU pragma: export

#undef OpenCookiesDialog

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_INFO_CHROME_PAGE_INFO_DELEGATE_H_
