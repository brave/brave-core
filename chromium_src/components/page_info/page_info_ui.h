/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PAGE_INFO_PAGE_INFO_UI_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PAGE_INFO_PAGE_INFO_UI_H_

#include "brave/components/ipfs/buildflags/buildflags.h"

#if BUILDFLAG(IPFS_ENABLED)
#define GetSecurityDescription                                           \
  GetSecurityDescription_ChromiumImpl(const IdentityInfo& identity_info) \
      const;                                                             \
  std::unique_ptr<SecurityDescription> GetSecurityDescription
#endif  // BUILDFLAG(IPFS_ENABLED)

#include "../../../../components/page_info/page_info_ui.h"

#if BUILDFLAG(IPFS_ENABLED)
#undef GetSecurityDescription
#endif  // BUILDFLAG(IPFS_ENABLED)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PAGE_INFO_PAGE_INFO_UI_H_
