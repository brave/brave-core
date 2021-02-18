/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/location.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace {

const char* kIpfs[] = {"ipfs", ".ipfs.localhost"};
const char* kIpns[] = {"ipns", ".ipns.localhost"};

// Convert https://{cid}.ipfs.localhost -> ipfs://{cid} origin if possible.
bool BuildRawIPFSIfApplicable(const String& host,
                              const String& scheme,
                              const String& ipfs_domain,
                              String* result) {
  if (!host.EndsWith(ipfs_domain))
    return false;
  wtf_size_t cid_length = host.Find(ipfs_domain);
  if (!cid_length || cid_length == kNotFound)
    return false;
  StringBuilder builder;
  builder.Append(scheme);
  builder.Append("://");
  builder.Append(host.Left(cid_length));
  *result = builder.ToString();
  return true;
}

}  // namespace

#define BRAVE_IPFS_ORIGIN                                                      \
  String ipfs_url;                                                             \
  if (BuildRawIPFSIfApplicable(Url().Host(), kIpfs[0], kIpfs[1], &ipfs_url) || \
      BuildRawIPFSIfApplicable(Url().Host(), kIpns[0], kIpns[1], &ipfs_url))   \
    return ipfs_url;

#include "../../../../../../../third_party/blink/renderer/core/frame/location.cc"
#undef BRAVE_IPFS_ORIGIN
