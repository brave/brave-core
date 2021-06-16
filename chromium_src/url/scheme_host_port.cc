/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

namespace {
const char kIPFSScheme[] = "ipfs";
const char kIPNSScheme[] = "ipns";
}  // namespace

// We do not canonicalize CIDv0(Qm...) and skip usual check for this
// https://docs.ipfs.io/concepts/content-addressing/#version-0-v0
#define BRAVE_IPFS_CIDv0                                       \
  if (scheme == kIPFSScheme || scheme == kIPNSScheme) {        \
    if (host.size() == 46 && host[0] == 'Q' && host[1] == 'm') \
      return true;                                             \
  }

#include "../../url/scheme_host_port.cc"
#undef BRAVE_IPFS_CIDv0
