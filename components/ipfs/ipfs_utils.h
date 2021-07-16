/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_

#include <string>

#include "components/version_info/channel.h"
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
GURL GetDefaultIPFSGateway(PrefService* prefs);
GURL GetAPIServer(version_info::Channel channel);
GURL ResolveWebUIFilesLocation(const std::string& directory,
                               version_info::Channel channel);
bool TranslateIPFSURI(const GURL& url,
                      GURL* new_url,
                      const GURL& gateway_url,
                      bool use_subdomain);
bool IsValidNodeFilename(const std::string& filename);

bool ParsePeerConnectionString(const std::string& value,
                               std::string* id,
                               std::string* address);
GURL ContentHashToCIDv1URL(const std::string& contenthash);
bool IsAPIGateway(const GURL& url, version_info::Channel channel);
bool IsIpfsResolveMethodDisabled(PrefService* prefs);
}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_UTILS_H_
