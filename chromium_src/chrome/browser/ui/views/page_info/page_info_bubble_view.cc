// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "components/grit/brave_components_strings.h"
#include "components/page_info/page_info.h"
#include "content/public/common/url_constants.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/magnet_protocol_handler.h"

// Show the InternalPageInfoBubbleView when viewing a page with the webtorrent
// scheme.
#define IsFileOrInternalPage(url) \
  IsFileOrInternalPage(url) || url.SchemeIs(url::kWebTorrentScheme)

// Set the text for the InternalPageInfoBubbleView on webtorrent: pages.
#define kFileScheme kWebTorrentScheme)) {  \
    text = IDS_PAGE_INFO_BRAVE_WEBTORRENT; \
  }                                        \
  else if (url.SchemeIs(url::kFileScheme

#endif

#include "src/chrome/browser/ui/views/page_info/page_info_bubble_view.cc"

#undef kFileScheme
#undef IsFileOrInternalPage
