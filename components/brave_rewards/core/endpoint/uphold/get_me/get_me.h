/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_GET_ME_GET_ME_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_GET_ME_GET_ME_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/uphold/uphold_user.h"

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

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoint {
namespace uphold {

using GetMeCallback =
    base::OnceCallback<void(mojom::Result result, internal::uphold::User user)>;

class GetMe {
 public:
  explicit GetMe(RewardsEngine& engine);
  ~GetMe();

  void Request(const std::string& token, GetMeCallback callback);

 private:
  std::string GetUrl();

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body,
                          internal::uphold::User* user);

  void OnRequest(GetMeCallback callback, mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngine> engine_;
};

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_GET_ME_GET_ME_H_
