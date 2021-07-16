/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/base32/base32.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace {

// Decodes a varint from the given string piece into the given int64_t. Returns
// if the  string had a valid varint (where a byte was found with it's top bit
// set).
bool DecodeVarInt(base::StringPiece* from, int64_t* into) {
  base::StringPiece::const_iterator it = from->begin();
  int shift = 0;
  uint64_t ret = 0;
  do {
    if (it == from->end())
      return false;

    // Shifting 64 or more bits is undefined behavior.
    DCHECK_LT(shift, 64);
    unsigned char c = *it;
    ret |= static_cast<uint64_t>(c & 0x7f) << shift;
    shift += 7;
  } while (*it++ & 0x80);
  *into = static_cast<int64_t>(ret);
  from->remove_prefix(it - from->begin());
  return true;
}

GURL AppendLocalPort(const std::string& port) {
  GURL gateway = GURL(ipfs::kDefaultIPFSLocalGateway);
  GURL::Replacements replacements;
  replacements.SetPortStr(port);
  return gateway.ReplaceComponents(replacements);
}

// RegEx to validate the node name:
// go-ipfs_v0.9.0-rc1_windows-amd64 - valid
// go-ipfs_v0.9.0_windows-amd64 - valid
constexpr char kExecutableRegEx[] =
    "go-ipfs_v(\\d+\\.\\d+\\.\\d+)(-rc\\d+)?\\_\\w+-amd64";

// Valid CID multibase prefix, "code" character
// from https://github.com/multiformats/multibase/blob/master/multibase.csv
const char kCIDv1Codes[] = "079fFvVtTbBcChkKzZmMuU";
const char kCIDv0Prefix[] = "Qm";

// Ipfs codes from multicodec table
// https://github.com/multiformats/multicodec/blob/master/table.csv
const int64_t kIpfsNSCodec = 0xE3;
const int64_t kIpnsNSCodec = 0xE5;

}  // namespace

namespace ipfs {

// Simple CID validation based on multibase table.
bool IsValidCID(const std::string& cid) {
  if (!cid.size())
    return false;
  if (!std::all_of(cid.begin(), cid.end(), [loc = std::locale{}](char c) {
        return std::isalnum(c, loc);
      }))
    return false;
  if (std::string(kCIDv1Codes).find(cid.at(0)) != std::string::npos)
    return true;
  return base::StartsWith(cid, kCIDv0Prefix);
}

bool IsIpfsResolveMethodDisabled(PrefService* prefs) {
  DCHECK(prefs);

  // Ignore the actual pref value if IPFS feature is disabled.
  if (IsIpfsDisabledByFeatureOrPolicy(prefs)) {
    return true;
  }

  return prefs->FindPreference(kIPFSResolveMethod) &&
         prefs->GetInteger(kIPFSResolveMethod) ==
             static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED);
}

bool IsIpfsMenuEnabled(PrefService* prefs) {
  return !ipfs::IsIpfsDisabledByFeatureOrPolicy(prefs) &&
         ipfs::IsLocalGatewayConfigured(prefs);
}

bool IsIpfsDisabledByFeatureOrPolicy(PrefService* prefs) {
  DCHECK(prefs);
  return (IsIpfsDisabledByPolicy(prefs) ||
          !base::FeatureList::IsEnabled(ipfs::features::kIpfsFeature));
}

bool IsIpfsDisabledByPolicy(PrefService* prefs) {
  DCHECK(prefs);
  return prefs->FindPreference(kIPFSEnabled) &&
         prefs->IsManagedPreference(kIPFSEnabled) &&
         !prefs->GetBoolean(kIPFSEnabled);
}

bool HasIPFSPath(const GURL& gurl) {
  const auto& path = gurl.path();
  return gurl.is_valid() && ((path.find("/ipfs/") != std::string::npos) ||
                             (path.find("/ipns/") != std::string::npos));
}

bool IsDefaultGatewayURL(const GURL& url, PrefService* prefs) {
  DCHECK(prefs);
  std::string gateway_host = GetDefaultIPFSGateway(prefs).host();
  return url.DomainIs(gateway_host) &&
         (HasIPFSPath(url) ||
          url.DomainIs(std::string("ipfs.") + gateway_host) ||
          url.DomainIs(std::string("ipns.") + gateway_host));
}

bool IsAPIGateway(const GURL& url, version_info::Channel channel) {
  if (!url.is_valid())
    return false;
  auto api_origin = ipfs::GetAPIServer(channel).GetOrigin();
  if (api_origin == url)
    return true;
  if (net::IsLocalhost(api_origin) && net::IsLocalhost(url)) {
    return api_origin.port() == url.port();
  }
  return false;
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

GURL ToPublicGatewayURL(const GURL& url, PrefService* prefs) {
  DCHECK(prefs);
  DCHECK(IsIPFSScheme(url) || IsLocalGatewayURL(url));
  GURL new_url;
  GURL gateway_url = GetDefaultIPFSGateway(prefs);
  // For ipfs/ipns schemes, use TranslateIPFSURI directly.
  if (IsIPFSScheme(url) &&
      TranslateIPFSURI(url, &new_url, gateway_url, false)) {
    return new_url;
  }

  // For local gateway addresses, replace its scheme, host, port with the
  // public gateway URL.
  if (IsLocalGatewayURL(url)) {
    GURL::Replacements replacements;
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

bool IsLocalGatewayConfigured(PrefService* prefs) {
  return static_cast<IPFSResolveMethodTypes>(prefs->GetInteger(
             kIPFSResolveMethod)) == IPFSResolveMethodTypes::IPFS_LOCAL;
}

GURL GetConfiguredBaseGateway(PrefService* prefs,
                              version_info::Channel channel) {
  return IsLocalGatewayConfigured(prefs)
             ? ::ipfs::GetDefaultIPFSLocalGateway(channel)
             : ::ipfs::GetDefaultIPFSGateway(prefs);
}

bool ResolveIPFSURI(PrefService* prefs,
                    version_info::Channel channel,
                    const GURL& ipfs_uri,
                    GURL* resolved_url) {
  CHECK(resolved_url);
  return ::ipfs::TranslateIPFSURI(
      ipfs_uri, resolved_url, GetConfiguredBaseGateway(prefs, channel), true);
}

GURL ipfs_default_gateway_for_test;

void SetIPFSDefaultGatewayForTest(const GURL& url) {
  ipfs_default_gateway_for_test = url;
}

GURL GetDefaultIPFSLocalGateway(version_info::Channel channel) {
  return AppendLocalPort(GetGatewayPort(channel));
}

GURL GetDefaultIPFSGateway(PrefService* prefs) {
  if (!ipfs_default_gateway_for_test.is_empty()) {
    return GURL(ipfs_default_gateway_for_test);
  }

  DCHECK(prefs);
  GURL gateway_url(prefs->GetString(kIPFSPublicGatewayAddress));
  if (gateway_url.DomainIs(kLocalhostIP)) {
    GURL::Replacements replacements;
    replacements.SetHostStr(kLocalhostDomain);
    return gateway_url.ReplaceComponents(replacements);
  }
  return gateway_url;
}

GURL GetAPIServer(version_info::Channel channel) {
  return AppendLocalPort(GetAPIPort(channel));
}


bool TranslateIPFSURI(const GURL& url,
                      GURL* new_url,
                      const GURL& gateway_url,
                      bool use_subdomain) {
  std::string cid = url.host();
  std::string path = url.path();
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

  return false;
}

GURL ResolveWebUIFilesLocation(const std::string& directory,
                               version_info::Channel channel) {
  GURL url = GetAPIServer(channel);
  GURL::Replacements replacements;
  replacements.SetPathStr("/webui/");
  std::string webui_files_ref = std::string("/files") + directory;
  replacements.SetRefStr(webui_files_ref);
  return url.ReplaceComponents(replacements);
}

// Extracts Address and PeerID from peer connection strings like:
// /ip4/104.131.131.82/udp/4001/quic/p2p/QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ
// /p2p/QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ
// QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ
bool ParsePeerConnectionString(const std::string& value,
                               std::string* id,
                               std::string* address) {
  if (!id || !address)
    return false;
  std::vector<std::string> parts = base::SplitStringUsingSubstr(
      value, "/p2p/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  std::string extracted_id = (parts.size() == 2) ? parts[1] : value;
  std::string extracted_address =
      (parts.size() == 2) ? parts[0] : std::string();

  bool valid_cid = IsValidCID(extracted_id);
  // For compatibility we allow PeerIDs started from 1,
  // like 12D3KooWBdmLJjhpgJ9KZgLM3f894ff9xyBfPvPjFNn7MKJpyrC2
  // only if p2p is present
  bool legacy_peer_id = (!extracted_id.empty() && !extracted_address.empty() &&
                         extracted_id.at(0) == '1');
  bool valid = valid_cid || legacy_peer_id;
  if (valid) {
    *id = extracted_id;
    *address = extracted_address;
  }
  return valid;
}

bool IsValidNodeFilename(const std::string& filename) {
  return RE2::FullMatch(filename, kExecutableRegEx);
}

GURL ContentHashToCIDv1URL(const std::string& contenthash) {
  int64_t code = 0;
  base::StringPiece input = contenthash;
  if (!DecodeVarInt(&input, &code))
    return GURL();
  if (code != kIpnsNSCodec && code != kIpfsNSCodec)
    return GURL();
  std::string encoded = base32::Base32Encode(input);
  if (encoded.empty())
    return GURL();
  std::string trimmed;
  base::TrimString(encoded, "=", &trimmed);
  std::string lowercase = base::ToLowerASCII(trimmed);
  // multibase format <base-encoding-character><base-encoded-data>
  // https://github.com/multiformats/multibase/blob/master/multibase.csv
  std::string cidv1 = "b" + lowercase;
  std::string scheme = (code == kIpnsNSCodec) ? kIPNSScheme : kIPFSScheme;
  return GURL(scheme + "://" + cidv1);
}

}  // namespace ipfs
