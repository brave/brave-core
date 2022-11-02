/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_

#include <string>

#include "components/version_info/channel.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

class PrefService;

namespace ipfs {

bool IsIpfsDisabledByFeatureOrPolicy(PrefService* prefs);
bool IsIpfsMenuEnabled(PrefService* prefs);
bool IsIpfsDisabledByPolicy(PrefService* prefs);
bool IsValidCID(const std::string& cid);
bool HasIPFSPath(const GURL& url);
bool IsDefaultGatewayURL(const GURL& url, PrefService* prefs);
bool IsLocalGatewayURL(const GURL& url);
bool IsIPFSScheme(const GURL& url);
// Extracts cid and path from ipfs URLs like:
// [scheme]://[cid][.gateway][/path]
// [scheme]://[cid][/path]
bool ParseCIDAndPathFromIPFSUrl(const GURL& url,
                                std::string* cid,
                                std::string* path);
GURL ToPublicGatewayURL(const GURL& url, PrefService* prefs);
GURL GetIPFSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url);
GURL GetIPNSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url);
bool IsLocalGatewayConfigured(PrefService* prefs);
GURL GetConfiguredBaseGateway(PrefService* prefs,
                              version_info::Channel channel);
bool ResolveIPFSURI(PrefService* prefs,
                    version_info::Channel channel,
                    const GURL& ipfs_uri,
                    GURL* resolved_url);
void SetIPFSDefaultGatewayForTest(const GURL& url);
GURL GetDefaultIPFSLocalGateway(version_info::Channel channel);
GURL GetDefaultNFTIPFSGateway(PrefService* prefs);
GURL GetDefaultIPFSGateway(PrefService* prefs);
GURL GetAPIServer(version_info::Channel channel);
GURL ResolveWebUIFilesLocation(const std::string& directory,
                               version_info::Channel channel);
bool TranslateIPFSURI(const GURL& url,
                      GURL* new_url,
                      const GURL& gateway_url,
                      bool use_subdomain);
absl::optional<GURL> TranslateXIPFSPath(const std::string& x_ipfs_path_header);
bool IsValidNodeFilename(const std::string& filename);

bool ParsePeerConnectionString(const std::string& value,
                               std::string* id,
                               std::string* address);
GURL ContentHashToCIDv1URL(base::span<const uint8_t> contenthash);
bool IsAPIGateway(const GURL& url, version_info::Channel channel);
bool IsIpfsResolveMethodDisabled(PrefService* prefs);
bool IsIpfsResolveMethodAsk(PrefService* prefs);
std::string GetRegistryDomainFromIPNS(const GURL& url);
bool IsValidCIDOrDomain(const std::string& value);
absl::optional<GURL> TranslateToCurrentGatewayUrl(const GURL& url);

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
