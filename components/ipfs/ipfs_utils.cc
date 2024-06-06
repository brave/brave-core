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
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {
constexpr char kIPFSScheme[] = "ipfs";
constexpr char kIPNSScheme[] = "ipns";
constexpr char kDefaultPublicGateway[] = "https://ipfs.io";

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

}  // namespace ipfs
