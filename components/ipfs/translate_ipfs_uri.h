/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_TRANSLATE_IPFS_URI_H_
#define BRAVE_COMPONENTS_IPFS_TRANSLATE_IPFS_URI_H_

#include "url/gurl.h"

namespace ipfs {

bool TranslateIPFSURI(const GURL& url, GURL* new_url, const GURL& gateway_url);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_TRANSLATE_IPFS_URI_H_
