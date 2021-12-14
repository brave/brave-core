/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_constants.h"

#define BRAVE_SHOULD_SHOW_URL_IPFS_CHECK \
  url.SchemeIs(ipfs::kIPFSScheme) || url.SchemeIs(ipfs::kIPNSScheme) ||

#include "src/chrome/browser/ui/views/location_bar/location_icon_view.cc"
#undef BRAVE_SHOULD_SHOW_URL_IPFS_CHECK
