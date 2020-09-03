/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_UPHOLD_GET_ME_GET_ME_H_
#define BRAVELEDGER_ENDPOINT_UPHOLD_GET_ME_GET_ME_H_

#include <string>

#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/ledger.h"

// GET https://api.uphold.com/v0/me
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "address": {
//     "city": "Anytown",
//     "line1": "123 Main Street",
//     "zipCode": "12345"
//   },
//   "birthdate": "1971-06-22",
//   "country": "US",
//   "email": "john@example.com",
//   "firstName": "John",
//   "fullName": "John Smith",
//   "id": "b34060c9-5ca3-4bdb-bc32-1f826ecea36e",
//   "identityCountry": "US",
//   "lastName": "Smith",
//   "name": "John Smith",
//   "settings": {
//     "currency": "USD",
//     "hasMarketingConsent": false,
//     "hasNewsSubscription": false,
//     "intl": {
//       "dateTimeFormat": {
//         "locale": "en-US"
//       },
//       "language": {
//         "locale": "en-US"
//       },
//       "numberFormat": {
//         "locale": "en-US"
//       }
//     },
//     "otp": {
//       "login": {
//         "enabled": true
//       },
//       "transactions": {
//         "transfer": {
//           "enabled": false
//         },
//         "send": {
//           "enabled": true
//         },
//         "withdraw": {
//           "crypto": {
//             "enabled": true
//           }
//         }
//       }
//     },
//     "theme": "vintage"
//   },
//   "memberAt": "2019-07-27T11:32:33.310Z",
//   "state": "US-MA",
//   "status": "ok",
//   "type": "individual",
//   "username": null,
//   "verifications": {
//     "termsEquities": {
//       "status": "required"
//     }
//   },
//   "balances": {
//     "available": "3.15",
//     "currencies": {
//       "BAT": {
//         "amount": "3.15",
//         "balance": "12.35",
//         "currency": "USD",
//         "rate": "0.25521"
//       }
//     },
//     "pending": "0.00",
//     "total": "3.15"
//   },
//   "currencies": [
//     "BAT"
//   ],
//   "phones": [
//     {
//       "e164Masked": "+XXXXXXXXX83",
//       "id": "8037c7ed-fe5a-4ad2-abfd-7c941f066cab",
//       "internationalMasked": "+X XXX-XXX-XX83",
//       "nationalMasked": "(XXX) XXX-XX83",
//       "primary": false,
//       "verified": false
//     }
//   ],
//   "tier": "other"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace uphold {

using GetMeCallback = std::function<void(
    const type::Result result,
    const ::ledger::uphold::User& user)>;

class GetMe {
 public:
  explicit GetMe(LedgerImpl* ledger);
  ~GetMe();

  void Request(
      const std::string& token,
      GetMeCallback callback);

 private:
  std::string GetUrl();

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      ::ledger::uphold::User* user);

  void OnRequest(
      const type::UrlResponse& response,
      GetMeCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_UPHOLD_GET_ME_GET_ME_H_
