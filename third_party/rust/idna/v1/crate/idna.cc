/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/check.h"
#include "base/i18n/icu_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/third_party/rust/idna/v1/crate/idna.h"
#include "brave/third_party/rust/idna/v1/crate/src/lib.rs.h"
#include "url/url_canon.h"

namespace idna {

void InitializeICUForTesting() {
  CHECK(base::i18n::InitializeICU());
}

IdnaResult DomainToASCII(rust::Str domain_str) {
  std::string domain(domain_str);
  IdnaResult res = { ""  /* domain */, false  /* valid */ };

  // Just return if it's already ascii
  if (base::IsStringASCII(domain)) {
    res.domain = domain;
    res.valid = true;
    return res;
  }

  // Otherwise try to convert it from IDN to punycode.
  url::RawCanonOutputT<char16_t, 256> punycode;
  if (!url::IDNToASCII(base::UTF8ToUTF16(domain), &punycode)) {
    return res;
  } else {
    res.domain = base::UTF16ToASCII(punycode.view());
    res.valid = true;
    return res;
  }
}

}  // namespace idna
