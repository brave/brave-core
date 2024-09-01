/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <string>
#include <string_view>

#include "base/logging.h"
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
    if (it == from.end()) {
      return {};
    }

    // Shifting 64 or more bits is undefined behavior.
    DCHECK_LT(shift, 64);
    unsigned char c = *it;
    ret |= static_cast<uint64_t>(c & 0x7f) << shift;
    shift += 7;
  } while (*it++ & 0x80);
  *into = static_cast<int64_t>(ret);
  return from.subspan(it - from.begin());
}

// Extracts cid and path from ipfs URLs like:
// [scheme]://[cid][.gateway][/path]
// [scheme]://[cid][/path]
bool ParseCIDAndPathFromIPFSUrl(const GURL& url,
                                std::string* cid,
                                std::string* path) {
  if (!url.SchemeIs(kIPFSScheme) && !url.SchemeIs(kIPNSScheme)) {
    return false;
  }
  if (url.host().empty() && url.path().find("/") != std::string::npos) {
    return false;
  }
  DCHECK(cid);
  DCHECK(path);
  // In the case of a URL like ipfs://[cid]/wiki/Vincent_van_Gogh.html
  // host is [cid] and path is /wiki/Vincent_van_Gogh.html
  if (!url.host().empty()) {
    *cid = url.host();
    *path = url.path();
  } else {
    *cid = url.path();
    *path = "";
  }
  return true;
}

// Simple CID validation based on multibase table.
bool IsValidCID(const std::string& cid) {
  if (cid.empty()) {
    return false;
  }
  return filecoin::is_valid_cid(cid);
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
