/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_constants.h"

#define BRAVE_GET_SHOW_TEXT \
  url.SchemeIs(ipfs::kIPFSScheme) || url.SchemeIs(ipfs::kIPNSScheme) || \
  url.SchemeIs(content::kBraveUIScheme) ||

#include "src/chrome/browser/ui/views/location_bar/location_icon_view.cc"
#undef BRAVE_GET_SHOW_TEXT
