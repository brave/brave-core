/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <vector>

#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_gateway.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/ipfs/translate_ipfs_uri.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace ipfs {

bool IsIpfsDisabledByPolicy(content::BrowserContext* context) {
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  return prefs->FindPreference(kIPFSEnabled) &&
         prefs->IsManagedPreference(kIPFSEnabled) &&
         !prefs->GetBoolean(kIPFSEnabled);
}

bool IsIpfsEnabled(content::BrowserContext* context) {
  if (context->IsOffTheRecord() || IsIpfsDisabledByPolicy(context) ||
      !base::FeatureList::IsEnabled(ipfs::features::kIpfsFeature)) {
    return false;
  }

  return true;
}

bool IsIpfsResolveMethodDisabled(content::BrowserContext* context) {
  // Ignore the actual pref value if IPFS feature is disabled.
  if (!IsIpfsEnabled(context)) {
    return true;
  }

  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  return prefs->FindPreference(kIPFSResolveMethod) &&
         prefs->GetInteger(kIPFSResolveMethod) ==
             static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED);
}

bool HasIPFSPath(const GURL& gurl) {
  static std::vector<URLPattern> url_patterns(
      {URLPattern(URLPattern::SCHEME_ALL, "*://*/ipfs/*"),
       URLPattern(URLPattern::SCHEME_ALL, "*://*/ipns/*")});
  return std::any_of(
      url_patterns.begin(), url_patterns.end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}

bool IsDefaultGatewayURL(const GURL& url) {
  std::string gateway_host = GetDefaultIPFSGateway().host();
  return url.DomainIs(gateway_host) &&
         (HasIPFSPath(url) ||
          url.DomainIs(std::string("ipfs.") + gateway_host) ||
          url.DomainIs(std::string("ipns.") + gateway_host));
}

bool IsLocalGatewayURL(const GURL& url) {
  return url.SchemeIsHTTPOrHTTPS() &&
         ((net::IsLocalhost(url) && HasIPFSPath(url)) ||
          url.DomainIs(std::string("ipfs.localhost")) ||
          url.DomainIs(std::string("ipns.localhost")));
}

bool IsIPFSScheme(const GURL& url) {
  return url.SchemeIs(kIPFSScheme) || url.SchemeIs(kIPNSScheme);
}

GURL ToPublicGatewayURL(const GURL& url) {
  DCHECK(IsIPFSScheme(url) || IsLocalGatewayURL(url));
  GURL new_url;

  // For ipfs/ipns schemes, use TranslateIPFSURI directly.
  if (IsIPFSScheme(url) &&
      TranslateIPFSURI(url, &new_url, GetDefaultIPFSGateway())) {
    return new_url;
  }

  // For local gateway addresses, replace its scheme, host, port with the
  // public gateway URL.
  if (IsLocalGatewayURL(url)) {
    GURL::Replacements replacements;
    GURL gateway_url = GetDefaultIPFSGateway();
    replacements.ClearPort();
    replacements.SetSchemeStr(gateway_url.scheme_piece());
    replacements.SetHostStr(gateway_url.host_piece());
    return url.ReplaceComponents(replacements);
  }

  return new_url;
}

GURL GetGatewayURL(const std::string& cid,
                   const std::string& path,
                   const GURL& base_gateway_url,
                   bool ipfs) {
  GURL uri(base_gateway_url);
  GURL::Replacements replacements;
  std::string host = base::StringPrintf("%s.%s.%s",
      cid.c_str(), ipfs? "ipfs" : "ipns", uri.host().c_str());
  replacements.SetHostStr(host);
  replacements.SetPathStr(path);
  return uri.ReplaceComponents(replacements);
}

GURL GetIPFSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url) {
  return GetGatewayURL(cid, path, base_gateway_url, true);
}

GURL GetIPNSGatewayURL(const std::string& cid,
                       const std::string& path,
                       const GURL& base_gateway_url) {
  return GetGatewayURL(cid, path, base_gateway_url, false);
}

}  // namespace ipfs
