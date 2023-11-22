/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_unittest_util.h"

namespace brave_ads::test {

std::string BuildRedeemPaymentTokensUrlResponseBody() {
  return R"(
            {
              "payload": "{"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}",
              "paymentCredentials": [
                {
                  "credential": {
                    "signature": "J6Lnoz1Ho5P4YDkcufA+WKUdR4C4f8QJARaT3Cko8RZ6dc777od9NQEaetU+xK3LXmQtmA6jfIUcLR3SCIJl0g==",
                    "t": "Z0GXil+GIQLOSSLHJV78jUE8cMxtwXtoROmv3uW8Qecpvx7L076GNI3TN44uF4uleOo2ZTpeKHzM2eeFHO2K6w=="
                  },
                  "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU="
                }
              ]
            })";
}

}  // namespace brave_ads::test
