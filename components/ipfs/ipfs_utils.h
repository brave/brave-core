/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_

#include <string>

namespace content {
class BrowserContext;
}  // namespace content

class GURL;

namespace ipfs {

// IsIpfsEnabled returns false if IPFS feature is unsupported for the given
// context, disabled by IPFSEnabled policy, or the feature flag.
bool IsIpfsEnabled(content::BrowserContext* context);
bool IsIpfsResolveMethodDisabled(content::BrowserContext* context);
bool IsIpfsDisabledByPolicy(content::BrowserContext* context);

bool HasIPFSPath(const GURL& url);
bool IsDefaultGatewayURL(const GURL& url);
bool IsLocalGatewayURL(const GURL& url);
bool IsIPFSScheme(const GURL& url);
GURL ToPublicGatewayURL(const GURL& url);
GURL GetIPFSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url);
GURL GetIPNSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
