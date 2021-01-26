/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/pref_names.h"

namespace unstoppable_domains {

// Used to determine which method should be used to resolve unstoppable
// domains, between:
// Disabled: Disable all unstoppable domains resolution.
// DNS Over HTTPS: Resolve domain name using a public DNS over HTTPS server.
const char kResolveMethod[] = "brave.unstoppable_domains.resolve_method";

}  // namespace unstoppable_domains
