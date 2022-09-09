/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_GET_CAPABILITIES_GET_CAPABILITIES_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_GET_CAPABILITIES_GET_CAPABILITIES_H_

#include <map>
#include <string>
#include <utility>

#include "bat/ledger/internal/uphold/uphold_capabilities.h"
#include "bat/ledger/ledger.h"

// GET https://api.uphold.com/v0/me/capabilities
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
// HTTP_TOO_MANY_REQUESTS (429)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// [
//   {
//     "category": "features",
//     "enabled": true,
//     "key": "change_phone",
//     "name": "Change Phone",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "features",
//     "enabled": true,
//     "key": "change_pii",
//     "name": "ChangePII",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "features",
//     "enabled": true,
//     "key": "equities",
//     "name": "Equities",
//     "requirements": [
//       "user-must-accept-equities-terms-of-services"
//     ],
//     "restrictions": []
//   },
//   {
//     "category": "features",
//     "enabled": true,
//     "key": "limit_orders",
//     "name": "Limit Orders",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "features",
//     "enabled": false,
//     "key": "physical_card_eea",
//     "name": "Physical Card EEA",
//     "requirements": [],
//     "restrictions": [
//       "user-country-not-supported"
//     ]
//   },
//   {
//     "category": "features",
//     "enabled": false,
//     "key": "physical_card_us",
//     "name": "Physical Card US",
//     "requirements": [],
//     "restrictions": [
//       "user-country-not-supported"
//     ]
//   },
//   {
//     "category": "features",
//     "enabled": false,
//     "key": "physical_card",
//     "name": "Physical Card",
//     "requirements": [],
//     "restrictions": [
//       "user-country-not-supported"
//     ]
//   },
//   {
//     "category": "features",
//     "enabled": true,
//     "key": "referrals",
//     "name": "Referrals",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "features",
//     "enabled": true,
//     "key": "staking",
//     "name": "Staking",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "features",
//     "enabled": true,
//     "key": "virtual_iban",
//     "name": "Virtual IBAN",
//     "requirements": [
//       "user-must-accept-virtual-iban-terms-of-services"
//     ],
//     "restrictions": []
//   },
//   {
//     "category": "permissions",
//     "enabled": true,
//     "key": "deposits",
//     "name": "Deposits",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "permissions",
//     "enabled": true,
//     "key": "invites",
//     "name": "Invites",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "permissions",
//     "enabled": true,
//     "key": "receives",
//     "name": "Receives",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "permissions",
//     "enabled": true,
//     "key": "sends",
//     "name": "Sends",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "permissions",
//     "enabled": true,
//     "key": "trades",
//     "name": "Trades",
//     "requirements": [],
//     "restrictions": []
//   },
//   {
//     "category": "permissions",
//     "enabled": true,
//     "key": "withdrawals",
//     "name": "Withdrawals",
//     "requirements": [],
//     "restrictions": []
//   }
// ]

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace uphold {

using GetCapabilitiesCallback =
    base::OnceCallback<void(mojom::Result, ledger::uphold::Capabilities)>;

class GetCapabilities {
 public:
  explicit GetCapabilities(LedgerImpl* ledger);
  ~GetCapabilities();

  void Request(const std::string& token, GetCapabilitiesCallback callback);

 private:
  struct Capability {
    bool enabled = false;
    bool requirements_empty = false;
  };

  using CapabilityMap = std::map<std::string, Capability>;

  void OnRequest(GetCapabilitiesCallback callback,
                 const mojom::UrlResponse& response);

  std::pair<mojom::Result, CapabilityMap> ProcessResponse(
      const mojom::UrlResponse& response);

  CapabilityMap ParseBody(const std::string& body);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_GET_CAPABILITIES_GET_CAPABILITIES_H_
