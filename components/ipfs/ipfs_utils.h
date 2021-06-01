/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_

#include <string>

#include "components/version_info/channel.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ipfs {

// IsIpfsEnabled returns false if IPFS feature is unsupported for the given
// context, disabled by IPFSEnabled policy, or the feature flag.
bool IsIpfsEnabled(content::BrowserContext* context);
bool IsIpfsResolveMethodDisabled(content::BrowserContext* context);
bool IsIpfsDisabledByPolicy(content::BrowserContext* context);
bool IsValidCID(const std::string& cid);
bool HasIPFSPath(const GURL& url);
bool IsDefaultGatewayURL(const GURL& url, content::BrowserContext* context);
bool IsLocalGatewayURL(const GURL& url);
bool IsIPFSScheme(const GURL& url);
// Extracts cid and path from ipfs URLs like:
// [scheme]://[cid][.gateway][/path]
// [scheme]://[cid][/path]
bool ParseCIDAndPathFromIPFSUrl(const GURL& url,
                                std::string* cid,
                                std::string* path);
GURL ToPublicGatewayURL(const GURL& url, content::BrowserContext* context);
GURL GetIPFSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url);
GURL GetIPNSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url);
bool IsLocalGatewayConfigured(content::BrowserContext* context);
GURL GetConfiguredBaseGateway(content::BrowserContext* context,
                              version_info::Channel channel);
bool ResolveIPFSURI(content::BrowserContext* context,
                    version_info::Channel channel,
                    const GURL& ipfs_uri,
                    GURL* resolved_url);
void SetIPFSDefaultGatewayForTest(const GURL& url);
GURL GetDefaultIPFSLocalGateway(version_info::Channel channel);
GURL GetDefaultIPFSGateway(content::BrowserContext* context);
GURL GetAPIServer(version_info::Channel channel);
GURL ResolveWebUIFilesLocation(const std::string& directory,
                               version_info::Channel channel);
bool TranslateIPFSURI(const GURL& url,
                      GURL* new_url,
                      const GURL& gateway_url,
                      bool use_subdomain);
bool IsIpfsMenuEnabled(content::BrowserContext* browser_context);
bool IsValidNodeFilename(const std::string& filename);

bool ParsePeerConnectionString(const std::string& value,
                               std::string* id,
                               std::string* address);
bool IsAPIGateway(const GURL& url, version_info::Channel channel);
}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
