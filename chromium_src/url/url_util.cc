/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"

namespace url {
struct Component;
}

namespace {
const char kIPFSScheme[] = "ipfs";
const char kIPNSScheme[] = "ipns";
}  // namespace

template <typename CHAR>
inline bool DoCompareSchemeComponent(const CHAR* spec,
                                     const url::Component& component,
                                     const char* compare_to);

#define ParsePathURL                                                          \
  if (DoCompareSchemeComponent(spec, scheme, kIPFSScheme) ||                  \
      DoCompareSchemeComponent(spec, scheme, kIPNSScheme)) {                  \
    ParseStandardURL(spec, spec_len, &parsed_input);                          \
    return CanonicalizeStandardURL(spec, spec_len, parsed_input, scheme_type, \
                                   charset_converter, output, output_parsed); \
  }                                                                           \
  ParsePathURL

#define ReplacePathURL                                              \
  (DoCompareSchemeComponent(spec, parsed.scheme, kIPFSScheme) ||    \
   DoCompareSchemeComponent(spec, parsed.scheme, kIPNSScheme))      \
      ? ReplaceStandardURL(spec, parsed, replacements, scheme_type, \
                           charset_converter, output, out_parsed)   \
      : ReplacePathURL

#include "../../url/url_util.cc"
#undef ReplacePathURL
#undef ParsePathURL
