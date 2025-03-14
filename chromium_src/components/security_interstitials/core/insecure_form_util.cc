/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define IsInsecureFormActionOnSecureSource \
  IsInsecureFormActionOnSecureSource_ChromiumImpl

#include "src/components/security_interstitials/core/insecure_form_util.cc"
#undef IsInsecureFormActionOnSecureSource

namespace {
constexpr char kOnionDomain[] = "onion";
}  // namespace

namespace security_interstitials {

bool IsInsecureFormActionOnSecureSource(const url::Origin& source_origin,
                                        const GURL& action_url) {
  if (source_origin.DomainIs(kOnionDomain)) {
    return IsInsecureFormAction(action_url);
  }

  return IsInsecureFormActionOnSecureSource_ChromiumImpl(source_origin,
                                                         action_url);
}

}  // namespace security_interstitials
