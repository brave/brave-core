/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_types.h"

namespace brave_ads::test {

namespace {

TokenIssuerInfo BuildTokenIssuer(
    const TokenIssuerType token_issuer_type,
    const TokenIssuerPublicKeyMap& token_issuer_public_keys) {
  TokenIssuerInfo token_issuer;
  token_issuer.type = token_issuer_type;
  token_issuer.public_keys = token_issuer_public_keys;

  return token_issuer;
}

}  // namespace

TokenIssuerList BuildTokenIssuers() {
  const TokenIssuerInfo confirmation_token_issuer =
      BuildTokenIssuer(TokenIssuerType::kConfirmations,
                       /*token_issuer_public_keys=*/
                       {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
                        {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}});

  const TokenIssuerInfo payment_token_issuer =
      BuildTokenIssuer(TokenIssuerType::kPayments,
                       /*token_issuer_public_keys=*/
                       {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                        {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  return {confirmation_token_issuer, payment_token_issuer};
}

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

IssuersInfo BuildIssuers(
    const int ping,
    const TokenIssuerPublicKeyMap& confirmation_token_issuer_public_keys,
    const TokenIssuerPublicKeyMap& payment_token_issuer_public_keys) {
  IssuersInfo issuers;

  issuers.ping = ping;

  if (!confirmation_token_issuer_public_keys.empty()) {
    const TokenIssuerInfo confirmation_token_issuer = BuildTokenIssuer(
        TokenIssuerType::kConfirmations, confirmation_token_issuer_public_keys);
    issuers.token_issuers.push_back(confirmation_token_issuer);
  }

  if (!payment_token_issuer_public_keys.empty()) {
    const TokenIssuerInfo payment_token_issuer = BuildTokenIssuer(
        TokenIssuerType::kPayments, payment_token_issuer_public_keys);
    issuers.token_issuers.push_back(payment_token_issuer);
  }

  return issuers;
}

IssuersInfo BuildIssuers() {
  return BuildIssuers(/*ping=*/7'200'000,
                      /*confirmation_token_issuer_public_keys=*/
                      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
                       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
                      /*payment_token_issuer_public_keys=*/
                      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});
}

void BuildAndSetIssuers() {
  SetIssuers(BuildIssuers());
}

}  // namespace brave_ads::test
