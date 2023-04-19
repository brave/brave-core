/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {

IssuerInfo BuildIssuer(const IssuerType type, const PublicKeyMap& public_keys) {
  IssuerInfo issuer;
  issuer.type = type;
  issuer.public_keys = public_keys;

  return issuer;
}

}  // namespace

URLResponseMap GetValidIssuersUrlResponses() {
  return {{// Issuers request
           "/v3/issuers/",
           {{net::HTTP_OK, /*response_body*/ R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
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
        }
        )"}}}};
}

URLResponseMap GetInvalidIssuersUrlResponses() {
  return {{// Get issuers request
           "/v3/issuers/",
           {{net::HTTP_OK, /*response_body*/ R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "6Orbju/jPQQGldu/MVyBi2wXKz8ynHIcdsbCWc9gGHQ=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "ECEKAGeRCNmAWimTs7fo0tTMcg8Kcmoy8w+ccOSYXT8=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "xp9WArE+RkSt579RCm6EhdmcW4RfS71kZHMgXpwgZyI=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "AE7e4Rh38yFmnyLyPYcyWKT//zLOsEEX+WdLZqvJxH0=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "HjID7G6LRrcRu5ezW0nLZtEARIBnjpaQFKTHChBuJm8=",
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
                },
                {
                  "publicKey": "XovQyvVWM8ez0mAzTtfqgPIbSpH5/idv8w0KJxhirwA=",
                  "associatedValue": "0.1"
                },
                {
                  "publicKey": "wAcnJtb34Asykf+2jrTWrjFiaTqilklZ6bxLyR3LyFo=",
                  "associatedValue": "0.1"
                },
                {
                  "publicKey": "ZvzeYOT1geUQXfOsYXBxZj/H26IfiBUVodHl51j68xI=",
                  "associatedValue": "0.1"
                },
                {
                  "publicKey": "JlOezORiqLkFkvapoNRGWcMH3/g09/7M2UPEwMjRpFE=",
                  "associatedValue": "0.1"
                },
                {
                  "publicKey": "hJP1nDjTdHcVDw347oH0XO+XBPPh5wZA2xWZE8QUSSA=",
                  "associatedValue": "0.1"
                },
                {
                  "publicKey": "+iyhYDv7W6cuFAD1tzsJIEQKEStTX9B/Tt62tqt+tG0=",
                  "associatedValue": "0.1"
                }
              ]
            }
          ]
        }
        )"}}}};
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
                      {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                       {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});
}

void BuildAndSetIssuers() {
  SetIssuers(BuildIssuers());
}

}  // namespace brave_ads
