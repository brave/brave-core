/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/common/extension_urls.h"

#define IsWebstoreUpdateUrl IsWebstoreUpdateUrl_ChromiumImpl
#include "src/extensions/common/extension_urls.cc"
#undef IsWebstoreUpdateUrl

namespace extension_urls {

namespace {

bool IsDefaultWebstoreUpdateUrl(const GURL& update_url) {
  GURL store_url = GetDefaultWebstoreUpdateUrl();
  return (update_url.host_piece() == store_url.host_piece() &&
          update_url.path_piece() == store_url.path_piece());
}

}  // namespace

bool IsWebstoreUpdateUrl(const GURL& update_url) {
  return IsDefaultWebstoreUpdateUrl(update_url) ||
         IsWebstoreUpdateUrl_ChromiumImpl(update_url);
}

}  // namespace extension_urls
