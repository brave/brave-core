/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/page_info/page_info_ui.h"
#include "brave/components/ipfs/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_utils.h"
#include "components/grit/brave_components_strings.h"
#endif  // BUILDFLAG(ENABLE_IPFS)

#if BUILDFLAG(ENABLE_IPFS)
#define GetSecurityDescription GetSecurityDescription_ChromiumImpl
#endif  // BUILDFLAG(ENABLE_IPFS)

#include "../../../../components/page_info/page_info_ui.cc"

#if BUILDFLAG(ENABLE_IPFS)
#undef GetSecurityDescription

std::unique_ptr<PageInfoUI::SecurityDescription>
PageInfoUI::GetSecurityDescription(const IdentityInfo& identity_info) const {
  if (!ipfs::IsIPFSScheme(GURL(identity_info.site_identity)))
    return GetSecurityDescription_ChromiumImpl(identity_info);
  return CreateSecurityDescription(
      SecuritySummaryColor::GREEN, IDS_PAGE_INFO_IPFS_BUBBLE_TITTLE,
      IDS_PAGE_INFO_IPFS_BUBBLE_TEXT, SecurityDescriptionType::CONNECTION);
}
#endif  // BUILDFLAG(ENABLE_IPFS)
