/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_STORE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_STORE_TEST_UTIL_H_

#include <optional>
#include <string_view>

#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/v2/credential_store.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"

namespace brave_vpn::v2::test {

// Matches an engaged std::optional<CredentialStore::Credential> whose value
// equals |value|. The expiration is not inspected.
testing::Matcher<const std::optional<CredentialStore::Credential>&>
CredentialIs(std::string_view value);

// Matches an engaged std::optional<CredentialStore::Credential> whose value
// equals |value|, and whose expiration equals |expiration|.
testing::Matcher<const std::optional<CredentialStore::Credential>&>
CredentialIs(std::string_view value, base::Time expiration);

}  // namespace brave_vpn::v2::test

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_CREDENTIAL_STORE_TEST_UTIL_H_
