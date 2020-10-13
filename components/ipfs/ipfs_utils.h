/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_

class GURL;

namespace ipfs {

bool IsIPFSURL(const GURL& url);
bool IsDefaultGatewayURL(const GURL& url);
bool IsLocalGatewayURL(const GURL& url);
bool IsIPFSScheme(const GURL& url);
GURL ToPublicGatewayURL(const GURL& url);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
