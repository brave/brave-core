/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#define BRAVE_PAGE_INFO_MAIN_VIEW_CALCULATE_PREFERRED_SIZE                     \
  if (presenter_ && ipfs::IsIPFSScheme(presenter_->site_url())) {              \
    width =                                                                    \
        std::max(security_container_view_->GetPreferredSize().width(), width); \
  }

#include "../../../../../../../chrome/browser/ui/views/page_info/page_info_main_view.cc"

#undef BRAVE_PAGE_INFO_MAIN_VIEW_CALCULATE_PREFERRED_SIZE
