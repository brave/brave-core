/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "url/url_canon.h"

namespace ipfs {
template <typename CHAR>
bool IpfsCIDv0(const url::URLComponentSource<CHAR>& source,
               const url::Parsed& parsed,
               url::CanonOutput* output,
               url::Parsed* new_parsed);
}  // namespace ipfs

#define CanonicalizeHost \
  ipfs::IpfsCIDv0<CHAR>(source, parsed, output, new_parsed) || CanonicalizeHost

#include "../../url/url_canon_stdurl.cc"
#undef CanonicalizeHost

namespace ipfs {
template <typename CHAR>
// Do not canonicalize CIDv0(Qm...) and copy it as is
bool IpfsCIDv0(const url::URLComponentSource<CHAR>& source,
               const url::Parsed& parsed,
               url::CanonOutput* output,
               url::Parsed* new_parsed) {
  if (parsed.host.len != 46 || (source.host[parsed.host.begin] != 'Q' ||
                                source.host[parsed.host.begin + 1] != 'm'))
    return false;
  if ((strncmp(&output->data()[new_parsed->scheme.begin], "ipfs",
               new_parsed->scheme.len) != 0) &&
      (strncmp(&output->data()[new_parsed->scheme.begin], "ipns",
               new_parsed->scheme.len) != 0))
    return false;
  new_parsed->host.begin = output->length();
  for (int i = 0; i < parsed.host.len; i++)
    output->push_back(source.host[i + parsed.host.begin]);

  new_parsed->host.len = parsed.host.len;
  return true;
}

}  // namespace ipfs
