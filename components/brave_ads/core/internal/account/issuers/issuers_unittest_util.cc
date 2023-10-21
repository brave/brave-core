/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"

namespace brave_ads::test {

namespace {

IssuerInfo BuildIssuer(const IssuerType type, const PublicKeyMap& public_keys) {
  IssuerInfo issuer;
  issuer.type = type;
  issuer.public_keys = public_keys;

  return issuer;
}

}  // namespace

std::string BuildIssuersUrlResponseBody() {
  return R"(
      {
        "ping": 7200000,
        "issuers": [
          {
            "name": "confirmations",
            "publicKeys": [
              {
                "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=",
                "associatedValue": ""
              },
              {
                "publicKey": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
                "associatedValue": ""
              }
            ]
          },
          {
            "name": "payments",
            "publicKeys": [
              {
                "publicKey": "JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=",
                "associatedValue": "0.0"
              },
              {
                "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "associatedValue": "0.1"
              }
            ]
          }
        ]
      })";
}

IssuersInfo BuildIssuers(const int ping,
                         const PublicKeyMap& confirmations_public_keys,
                         const PublicKeyMap& payments_public_keys) {
  IssuersInfo issuers;

  issuers.ping = ping;

  if (!confirmations_public_keys.empty()) {
    const IssuerInfo confirmations_issuer =
        BuildIssuer(IssuerType::kConfirmations, confirmations_public_keys);
    issuers.issuers.push_back(confirmations_issuer);
  }

  if (!payments_public_keys.empty()) {
    const IssuerInfo payments_issuer =
        BuildIssuer(IssuerType::kPayments, payments_public_keys);
    issuers.issuers.push_back(payments_issuer);
  }

  return issuers;
}

IssuersInfo BuildIssuers() {
  return BuildIssuers(7'200'000,
                      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
                       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
                      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});
}

void BuildAndSetIssuers() {
  SetIssuers(BuildIssuers());
}

}  // namespace brave_ads::test
