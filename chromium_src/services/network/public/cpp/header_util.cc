/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Adds Sec-GPC to exceptions in ContainsForbiddenSecurityHeader. Otherwise,
// it gets flagged as a "Forbidden Sec- header from renderer" by
// CorsURLLoaderFactory.
#define BRAVE_CONTAINS_FORBIDDEN_SECURITY_HEADER           \
  if (base::EqualsCaseInsensitiveASCII(name, "Sec-GPC")) { \
    return value == "1";                                   \
  }

#include <services/network/public/cpp/header_util.cc>
#undef BRAVE_CONTAINS_FORBIDDEN_SECURITY_HEADER
