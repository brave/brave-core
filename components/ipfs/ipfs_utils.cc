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
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace {

GURL AppendLocalPort(const std::string& port) {
  GURL gateway = GURL(ipfs::kDefaultIPFSLocalGateway);
  GURL::Replacements replacements;
  replacements.SetPortStr(port);
  return gateway.ReplaceComponents(replacements);
}

}  // namespace

namespace ipfs {

bool IsIpfsDisabledByPolicy(content::BrowserContext* context) {
  DCHECK(context);
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  return prefs->FindPreference(kIPFSEnabled) &&
         prefs->IsManagedPreference(kIPFSEnabled) &&
         !prefs->GetBoolean(kIPFSEnabled);
}

bool IsIpfsEnabled(content::BrowserContext* context) {
  DCHECK(context);
  if (context->IsOffTheRecord() || IsIpfsDisabledByPolicy(context) ||
      !base::FeatureList::IsEnabled(ipfs::features::kIpfsFeature)) {
    return false;
  }

  return true;
}

bool IsIpfsResolveMethodDisabled(content::BrowserContext* context) {
  DCHECK(context);

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

bool IsDefaultGatewayURL(const GURL& url, content::BrowserContext* context) {
  DCHECK(context);
  std::string gateway_host = GetDefaultIPFSGateway(context).host();
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

GURL ToPublicGatewayURL(const GURL& url, content::BrowserContext* context) {
  DCHECK(context);
  DCHECK(IsIPFSScheme(url) || IsLocalGatewayURL(url));
  GURL new_url;

  // For ipfs/ipns schemes, use TranslateIPFSURI directly.
  if (IsIPFSScheme(url) &&
      TranslateIPFSURI(url, &new_url, GetDefaultIPFSGateway(context), false)) {
    return new_url;
  }

  // For local gateway addresses, replace its scheme, host, port with the
  // public gateway URL.
  if (IsLocalGatewayURL(url)) {
    GURL::Replacements replacements;
    GURL gateway_url = GetDefaultIPFSGateway(context);
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
  std::string host = base::StringPrintf(
      "%s.%s.%s", cid.c_str(), ipfs ? "ipfs" : "ipns", uri.host().c_str());
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

bool IsLocalGatewayConfigured(content::BrowserContext* context) {
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  return static_cast<IPFSResolveMethodTypes>(prefs->GetInteger(
             kIPFSResolveMethod)) == IPFSResolveMethodTypes::IPFS_LOCAL;
}

GURL GetConfiguredBaseGateway(content::BrowserContext* context,
                              version_info::Channel channel) {
  return IsLocalGatewayConfigured(context)
             ? ::ipfs::GetDefaultIPFSLocalGateway(channel)
             : ::ipfs::GetDefaultIPFSGateway(context);
}

bool ResolveIPFSURI(content::BrowserContext* context,
                    version_info::Channel channel,
                    const GURL& ipfs_uri,
                    GURL* resolved_url) {
  CHECK(resolved_url);
  return ::ipfs::TranslateIPFSURI(
      ipfs_uri, resolved_url, GetConfiguredBaseGateway(context, channel), true);
}

GURL ipfs_default_gateway_for_test;

void SetIPFSDefaultGatewayForTest(const GURL& url) {
  ipfs_default_gateway_for_test = url;
}

GURL GetDefaultIPFSLocalGateway(version_info::Channel channel) {
  return AppendLocalPort(GetGatewayPort(channel));
}

GURL GetDefaultIPFSGateway(content::BrowserContext* context) {
  if (!ipfs_default_gateway_for_test.is_empty()) {
    return GURL(ipfs_default_gateway_for_test);
  }

  DCHECK(context);
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  return GURL(prefs->GetString(kIPFSPublicGatewayAddress));
}

GURL GetAPIServer(version_info::Channel channel) {
  return AppendLocalPort(GetAPIPort(channel));
}

bool TranslateIPFSURI(const GURL& url,
                      GURL* new_url,
                      const GURL& gateway_url,
                      bool use_subdomain) {
  if (!url.SchemeIs(kIPFSScheme) && !url.SchemeIs(kIPNSScheme)) {
    return false;
  }

  std::string path = url.path();
  // In the case of a URL like ipfs://[cid]/wiki/Vincent_van_Gogh.html
  // host is empty and path is //wiki/Vincent_van_Gogh.html
  if (url.host().empty() && path.length() > 2 && path.substr(0, 2) == "//") {
    std::string cid(path.substr(2));
    // If we have a path after the CID, get at the real resource path
    size_t pos = cid.find("/");
    std::string path;
    if (pos != std::string::npos && pos != 0) {
      // path would be /wiki/Vincent_van_Gogh.html
      path = cid.substr(pos, cid.length() - pos);
      // cid would be [cid]
      cid = cid.substr(0, pos);
    }
    bool ipfs_scheme = url.scheme() == kIPFSScheme;
    bool ipns_scheme = url.scheme() == kIPNSScheme;
    if ((ipfs_scheme && std::all_of(cid.begin(), cid.end(),
                                    [loc = std::locale{}](char c) {
                                      return std::isalnum(c, loc);
                                    })) ||
        ipns_scheme) {
      // new_url would be:
      // https://dweb.link/ipfs/[cid]//wiki/Vincent_van_Gogh.html
      if (new_url) {
        GURL::Replacements replacements;
        replacements.SetSchemeStr(gateway_url.scheme_piece());
        replacements.SetPortStr(gateway_url.port_piece());
        std::string new_host = gateway_url.host();
        std::string new_path = path;
        if (use_subdomain) {
          new_host = base::StringPrintf("%s.%s.%s", cid.c_str(),
                                        ipfs_scheme ? "ipfs" : "ipns",
                                        gateway_url.host().c_str());
        } else {
          new_path = (ipfs_scheme ? "ipfs/" : "ipns/") + cid + path;
        }
        replacements.SetHostStr(new_host);
        replacements.SetPathStr(new_path);
        *new_url = url.ReplaceComponents(replacements);
        VLOG(1) << "[IPFS] " << __func__ << " new URL: " << *new_url;
      }

      return true;
    }
  }
  return false;
}

}  // namespace ipfs
