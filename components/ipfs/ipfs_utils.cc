/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/filecoin/rs/src/lib.rs.h"
#include "components/base32/base32.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {
constexpr char kIPFSScheme[] = "ipfs";
constexpr char kIPNSScheme[] = "ipns";
constexpr char kDefaultPublicGateway[] = "https://ipfs.io";

// Ipfs codes from multicodec table
// https://github.com/multiformats/multicodec/blob/master/table.csv
const int64_t kIpfsNSCodec = 0xE3;
const int64_t kIpnsNSCodec = 0xE5;

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

bool ParseCIDAndPathFromIPFSUrl(const GURL& url,
                                std::string* cid,
                                std::string* path) {
  if (!url.SchemeIs(kIPFSScheme) && !url.SchemeIs(kIPNSScheme)) {
    return false;
  }
  if (!url.host().empty()) {
    return false;
  }
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

// Simple CID validation based on multibase table.
bool IsValidCID(const std::string& cid) {
  if (!cid.size()) {
    return false;
  }
  return filecoin::is_valid_cid(cid);
}

bool IsValidIPNSCID(const std::string& cid) {
  return IsValidCID(cid) && cid.at(0) == 'k';
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

std::optional<GURL> ExtractSourceFromGatewayHost(const GURL& url) {
  std::vector<std::string> host_parts = base::SplitStringUsingSubstr(
      url.host(), ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (host_parts.size() <= 2) {
    return std::nullopt;
  }
  GURL final_url;
  if (host_parts.at(1) == "ipfs" && IsValidCID(host_parts.at(0))) {
    final_url = GURL("ipfs://" + host_parts.at(0) + url.path());
  } else if (host_parts.at(1) == "ipns" && IsValidIPNSCID(host_parts.at(0))) {
    final_url = GURL("ipns://" + host_parts.at(0) + url.path());
  } else if (host_parts.at(1) == "ipns") {
    std::string decoded = DecodeSingleLabelForm(host_parts.at(0));
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

  if (path_parts.at(0) == "ipfs" && IsValidCID(path_parts.at(1))) {
    final_url = GURL("ipfs://" + path_parts.at(1) + final_path);
  } else if (path_parts.at(0) == "ipns" && IsValidIPNSCID(path_parts.at(1))) {
    final_url = GURL("ipns://" + path_parts.at(1) + final_path);
  } else if (path_parts.at(0) == "ipns") {
    std::string decoded = DecodeSingleLabelForm(path_parts.at(1));
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

bool TranslateIPFSURI(const GURL& url, GURL* new_url, bool use_subdomain) {
  const GURL gateway_url{kDefaultPublicGateway};
  std::string cid, path;
  if (!ParseCIDAndPathFromIPFSUrl(url, &cid, &path)) {
    return false;
  }
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

GURL GetDefaultIPFSGateway() {
  return GURL(kDefaultPublicGateway);
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

GURL ContentHashToCIDv1URL(base::span<const uint8_t> contenthash) {
  int64_t code = 0;
  contenthash = DecodeVarInt(contenthash, &code);
  if (contenthash.empty()) {
    return GURL();
  }
  if (code != kIpnsNSCodec && code != kIpfsNSCodec) {
    return GURL();
  }
  std::string encoded = base32::Base32Encode(contenthash);
  if (encoded.empty()) {
    return GURL();
  }
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
