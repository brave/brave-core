/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_VIEW_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_VIEW_FACTORY_H_

#include "brave/components/ipfs/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_IPFS)
#define CreateSecurityPageView           \
  CreateSecurityPageView_ChromiumImpl(); \
  std::unique_ptr<views::View> CreateSecurityPageView
#endif  // BUILDFLAG(ENABLE_IPFS)

#include "../../../../../../../chrome/browser/ui/views/page_info/page_info_view_factory.h"

#if BUILDFLAG(ENABLE_IPFS)
#undef CreateSecurityPageView
#endif  // BUILDFLAG(ENABLE_IPFS)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_VIEW_FACTORY_H_
