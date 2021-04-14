/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/buildflags/buildflags.h"
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

#define kPageInfoHelpCenterURL GetIPFSLearnMoreURL(web_contents()->GetURL())
#endif  // BUILDFLAG(IPFS_ENABLED)

#include "../../../../../../../chrome/browser/ui/views/page_info/page_info_bubble_view.cc"

#if BUILDFLAG(IPFS_ENABLED)
#undef kPageInfoHelpCenterURL
#endif  // BUILDFLAG(IPFS_ENABLED)
