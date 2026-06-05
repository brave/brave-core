/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/test/issuers_test_util.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"

namespace brave_ads::test {

std::string BuildIssuersUrlResponseBody() {
  return R"JSON(
      {
        "ping": 7200000,
        "issuers": [
          {
            "name": "confirmations",
            "publicKeys": [
              {
                "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
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
                "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
                "associatedValue": "0.1"
              }
            ]
          }
        ]
      })JSON";
}

IssuersInfo BuildIssuers(int ping,
                         base::flat_set<std::string> confirmation_public_keys,
                         PaymentTokenIssuerPublicKeyMap payment_public_keys) {
  IssuersInfo issuers;
  issuers.ping = ping;
  issuers.confirmation_token_issuer.public_keys =
      std::move(confirmation_public_keys);
  issuers.payment_token_issuer.public_keys = std::move(payment_public_keys);
  return issuers;
}

IssuersInfo BuildIssuers() {
  return BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_public_keys=*/
      {"OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
       "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="},
      /*payment_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=", 0.1}});
}

void BuildAndSetIssuers() {
  SetIssuers(BuildIssuers());
}

}  // namespace brave_ads::test
