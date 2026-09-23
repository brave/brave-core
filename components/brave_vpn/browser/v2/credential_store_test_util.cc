/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_store_test_util.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_vpn::v2::test {
namespace {
using Credential = CredentialStore::Credential;
}  // namespace

testing::Matcher<const std::optional<Credential>&> CredentialIs(
    std::string_view value) {
  return testing::Optional(
      testing::Field("value", &Credential::value, std::string(value)));
}

testing::Matcher<const std::optional<Credential>&> CredentialIs(
    std::string_view value,
    base::Time expiration) {
  return testing::Optional(testing::AllOf(
      testing::Field("value", &Credential::value, std::string(value)),
      testing::Field("expiration", &Credential::expiration, expiration)));
}

}  // namespace brave_vpn::v2::test
