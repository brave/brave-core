/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/feature_list.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/filecoin/rs/src/lib.rs.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/base32/base32.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

// RegEx to validate the node name:
// go-ipfs_v0.9.0-rc1_windows-amd64 - valid
// go-ipfs_v0.9.0_windows-amd64 - valid
constexpr char kExecutableRegEx[] =
    "go-ipfs_v(\\d+\\.\\d+\\.\\d+)(-rc\\d+)?\\_\\w+-\\w+";

// Ipfs codes from multicodec table
// https://github.com/multiformats/multicodec/blob/master/table.csv
const int64_t kIpfsNSCodec = 0xE3;
const int64_t kIpnsNSCodec = 0xE5;

GURL AppendLocalPort(const std::string& port) {
  GURL gateway = GURL(ipfs::kDefaultIPFSLocalGateway);
  GURL::Replacements replacements;
  replacements.SetPortStr(port);
  return gateway.ReplaceComponents(replacements);
}

std::optional<GURL> ExtractSourceFromGatewayHost(const GURL& url) {
  std::vector<std::string> host_parts = base::SplitStringUsingSubstr(
      url.host(), ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (host_parts.size() <= 2) {
    return std::nullopt;
  }
  GURL final_url;
  if (host_parts.at(1) == "ipfs" && ipfs::IsValidCID(host_parts.at(0))) {
    final_url = GURL("ipfs://" + host_parts.at(0) + url.path());
  } else if (host_parts.at(1) == "ipns" &&
             ipfs::IsValidIPNSCID(host_parts.at(0))) {
    final_url = GURL("ipns://" + host_parts.at(0) + url.path());
  } else if (host_parts.at(1) == "ipns") {
    std::string decoded = ipfs::DecodeSingleLabelForm(host_parts.at(0));
    final_url = GURL("https://" + decoded + url.path());
  }

  if (!final_url.is_valid()) {
    return std::nullopt;
  }

  GURL::Replacements replacements;
  replacements.SetQueryStr(url.query_piece());
  replacements.SetRefStr(url.ref_piece());
  return final_url.ReplaceComponents(replacements);
}

std::optional<GURL> ExtractSourceFromGatewayPath(const GURL& url) {
  std::vector<std::string> path_parts = base::SplitStringUsingSubstr(
      url.path(), "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (path_parts.size() < 2) {
    return std::nullopt;
  }
  GURL final_url;

  std::string final_path;
  if (path_parts.size() >= 3) {
    std::vector<std::string> final_path_parts(path_parts.begin() + 2,
                                              path_parts.end());
    final_path = "/" + base::JoinString(final_path_parts, "/");
  }

  if (path_parts.at(0) == "ipfs" && ipfs::IsValidCID(path_parts.at(1))) {
    final_url = GURL("ipfs://" + path_parts.at(1) + final_path);
  } else if (path_parts.at(0) == "ipns" &&
             ipfs::IsValidIPNSCID(path_parts.at(1))) {
    final_url = GURL("ipns://" + path_parts.at(1) + final_path);
  } else if (path_parts.at(0) == "ipns") {
    std::string decoded = ipfs::DecodeSingleLabelForm(path_parts.at(1));
    final_url = GURL("https://" + decoded + final_path);
  }

  if (!final_url.is_valid()) {
    return std::nullopt;
  }

  GURL::Replacements replacements;
  replacements.SetQueryStr(url.query_piece());
  replacements.SetRefStr(url.ref_piece());
  return final_url.ReplaceComponents(replacements);
}

}  // namespace

namespace ipfs {

// Simple CID validation based on multibase table.
bool IsValidCID(const std::string& cid) {
  if (!cid.size())
    return false;
  return filecoin::is_valid_cid(cid);
}

bool IsValidIPNSCID(const std::string& cid) {
  return IsValidCID(cid) && cid.at(0) == 'k';
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

bool IsIpfsResolveMethodAsk(PrefService* prefs) {
  DCHECK(prefs);

  // Ignore the actual pref value if IPFS feature is disabled.
  if (IsIpfsResolveMethodDisabled(prefs)) {
    return false;
  }

  return prefs->FindPreference(kIPFSResolveMethod) &&
         prefs->GetInteger(kIPFSResolveMethod) ==
             static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK);
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

std::optional<GURL> TranslateXIPFSPath(const std::string& x_ipfs_path_header) {
  std::string scheme;
  if (base::StartsWith(x_ipfs_path_header, "/ipfs/")) {
    scheme = kIPFSScheme;
  } else if (base::StartsWith(x_ipfs_path_header, "/ipns/")) {
    scheme = kIPNSScheme;
  } else {
    return std::nullopt;
  }
  std::string content = x_ipfs_path_header.substr(6, x_ipfs_path_header.size());
  if (content.empty()) {
    return std::nullopt;
  }
  GURL result = GURL(scheme + "://" + content);
  if (!result.is_valid()) {
    return std::nullopt;
  }
  return result;
}

bool IsAPIGateway(const GURL& url, version_info::Channel channel) {
  if (!url.is_valid())
    return false;
  auto api_origin_url =
      url::Origin::Create(ipfs::GetAPIServer(channel)).GetURL();
  if (api_origin_url == url)
    return true;
  if (net::IsLocalhost(api_origin_url) && net::IsLocalhost(url)) {
    return api_origin_url.port() == url.port();
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

bool SetDefaultNFTIPFSGateway(PrefService* prefs, const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }
  DCHECK(prefs);
  prefs->SetString(kIPFSPublicNFTGatewayAddress, url.spec());
  return true;
}

bool SetDefaultIPFSGateway(PrefService* prefs, const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }
  DCHECK(prefs);
  prefs->SetString(kIPFSPublicGatewayAddress, url.spec());
  return true;
}

GURL GetDefaultNFTIPFSGateway(PrefService* prefs) {
  if (!ipfs_default_gateway_for_test.is_empty()) {
    return GURL(ipfs_default_gateway_for_test);
  }

  DCHECK(prefs);
  GURL gateway_url(prefs->GetString(kIPFSPublicNFTGatewayAddress));
  if (gateway_url.DomainIs(kLocalhostIP)) {
    GURL::Replacements replacements;
    replacements.SetHostStr(kLocalhostDomain);
    return gateway_url.ReplaceComponents(replacements);
  }
  return gateway_url;
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

bool ParseCIDAndPathFromIPFSUrl(const GURL& url,
                                std::string* cid,
                                std::string* path) {
  if (!url.SchemeIs(kIPFSScheme) && !url.SchemeIs(kIPNSScheme)) {
    return false;
  }
  if (!url.host().empty())
    return false;
  DCHECK(cid);
  DCHECK(path);
  // ipfs: or ipfs://
  size_t offset = (url.path().substr(0, 2) == "//") ? 2 : 0;
  // In the case of a URL like ipfs://[cid]/wiki/Vincent_van_Gogh.html
  // host is empty and path is //wiki/Vincent_van_Gogh.html
  std::string local_cid(url.path().substr(offset));
  // If we have a path after the CID, get at the real resource path
  size_t pos = local_cid.find("/");
  if (pos != std::string::npos && pos != 0) {
    // path would be /wiki/Vincent_van_Gogh.html
    *path = local_cid.substr(pos, local_cid.length() - pos);

    // cid would be [cid]
    *cid = local_cid.substr(0, pos);
    return true;
  }
  *cid = local_cid;
  return true;
}

bool TranslateIPFSURI(const GURL& url,
                      GURL* new_url,
                      const GURL& gateway_url,
                      bool use_subdomain) {
  std::string cid, path;
  if (!ParseCIDAndPathFromIPFSUrl(url, &cid, &path))
    return false;
  bool ipfs_scheme = url.scheme() == kIPFSScheme;
  bool ipns_scheme = url.scheme() == kIPNSScheme;
  if ((ipfs_scheme && IsValidCID(cid)) || ipns_scheme) {
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

GURL ContentHashToCIDv1URL(base::span<const uint8_t> contenthash) {
  int64_t code = 0;
  contenthash = DecodeVarInt(contenthash, &code);
  if (contenthash.empty())
    return GURL();
  if (code != kIpnsNSCodec && code != kIpfsNSCodec)
    return GURL();
  std::string encoded = base32::Base32Encode(contenthash);
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

// Decodes a varint from the given string piece into the given int64_t. Returns
// remaining span if the string had a valid varint (where a byte was found with
// it's top bit set).
base::span<const uint8_t> DecodeVarInt(base::span<const uint8_t> from,
                                       int64_t* into) {
  auto it = from.begin();
  int shift = 0;
  uint64_t ret = 0;
  do {
    if (it == from.end())
      return {};

    // Shifting 64 or more bits is undefined behavior.
    DCHECK_LT(shift, 64);
    unsigned char c = *it;
    ret |= static_cast<uint64_t>(c & 0x7f) << shift;
    shift += 7;
  } while (*it++ & 0x80);
  *into = static_cast<int64_t>(ret);
  return from.subspan(it - from.begin());
}

bool IsValidCIDOrDomain(const std::string& value) {
  if (ipfs::IsValidCID(value))
    return true;
  auto domain = GetDomainAndRegistry(
      value, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return !domain.empty();
}

std::string GetRegistryDomainFromIPNS(const GURL& url) {
  if (!url.SchemeIs(ipfs::kIPNSScheme))
    return std::string();
  std::string cid;
  std::string ipfs_path;
  if (!ipfs::ParseCIDAndPathFromIPFSUrl(url, &cid, &ipfs_path) || cid.empty())
    return std::string();
  return GetDomainAndRegistry(
      cid, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

// https://github.com/ipfs/specs/blob/main/http-gateways/SUBDOMAIN_GATEWAY.md#host-request-header
std::string DecodeSingleLabelForm(const std::string& input) {
  // Normal form URLs such as en.wikipedia-on-ipfs.org should stay as is
  if (input.find('.') != std::string::npos) {
    return input;
  }
  std::string result;
  const char* chars = input.c_str();
  size_t i = 0;
  for (i = 0; i < input.size(); i++) {
    if (chars[i] == '-' && (i < input.size() - 1) && chars[i + 1] == '-') {
      result.push_back('-');
      i++;
    } else if (chars[i] == '-') {
      result.push_back('.');
    } else {
      result.push_back(chars[i]);
    }
  }
  return result;
}

// Subdomain based gateway URL:
// 1) CID:
// bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy.ipfs.gateway.io ->
// ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy
// 2) Single-label domain:
// en-wikipedia--on--ipfs-org.ipns.gateway.io ->
// https://en.wikipedia-on-ipfs.org
// Path based gateway URL:
// 1) CID:
// gateway.io/ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy ->
// ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy
// 2) Non-single-label domain:
// gateway.io/ipns/en.wikipedia-on-ipfs.org ->
// https://en.wikipedia-on-ipfs.org
// 3) Single-label domain:
// gateway.io/ipns/en-wikipedia--on--ipfs-org ->
// https://en.wikipedia-on-ipfs.org
// 4) IPNS key:
// gateway.io/ipns/k2k4r8k4oiuzuccssu5jj27hrth43yqoq55wvm46e7ygqokvlz4ixmfn ->
// ipns://k2k4r8k4oiuzuccssu5jj27hrth43yqoq55wvm46e7ygqokvlz4ixmfn
std::optional<GURL> ExtractSourceFromGateway(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS()) {
    return std::nullopt;
  }

  auto result = ExtractSourceFromGatewayHost(url);
  if (result) {
    return result;
  }

  return ExtractSourceFromGatewayPath(url);
}

}  // namespace ipfs
