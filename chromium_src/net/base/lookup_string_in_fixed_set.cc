/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LookupSuffixInReversedSet LookupSuffixInReversedSet_ChromiumImpl
#include "../../../../net/base/lookup_string_in_fixed_set.cc"
#undef LookupSuffixInReversedSet

#include "base/strings/string_util.h"

namespace net {

int LookupSuffixInReversedSet(const unsigned char* graph,
                              size_t length,
                              bool include_private,
                              base::StringPiece host,
                              size_t* suffix_length) {
  // Special cases to be treated as public suffixes for security concern.
  // With this, {CID}.ipfs.localhost with different CIDs cannot share cookies.
  if (base::EndsWith(host, ".ipfs.localhost") ||
      base::EndsWith(host, ".ipns.localhost")) {
    *suffix_length = 14;
    return kDafsaFound;
  }

  return LookupSuffixInReversedSet_ChromiumImpl(
      graph, length, include_private, host, suffix_length);
}

}  // namespace net
