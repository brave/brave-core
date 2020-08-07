/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_uphold.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/logging/logging.h"
#include "net/http/http_status_code.h"

namespace {

braveledger_uphold::UserStatus GetUserStatus(const std::string& status) {
  if (status == "pending") {
    return braveledger_uphold::UserStatus::PENDING;
  }

  if (status == "restricted") {
    return braveledger_uphold::UserStatus::RESTRICTED;
  }

  if (status == "blocked") {
    return braveledger_uphold::UserStatus::BLOCKED;
  }

  if (status == "ok") {
    return braveledger_uphold::UserStatus::OK;
  }

  return braveledger_uphold::UserStatus::EMPTY;
}

}  // namespace

namespace braveledger_response_util {

// Request Url:
// GET https://api.uphold.com/v0/me/cards/{wallet_address}
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "CreatedByApplicationId": "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
//   "address": {
//     "wire": "XXXXXXXXXX"
//   },
//   "available": "0.00",
//   "balance": "0.00",
//   "currency": "BAT",
//   "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//   "label": "Brave Browser",
//   "lastTransactionAt": null,
//   "settings": {
//     "position": 1,
//     "protected": false,
//     "starred": true
//   },
//   "createdByApplicationClientId": "4c2b665ca060d912fec5c735c734859a06118cc8",
//   "normalized": [
//     {
//       "available": "0.00",
//       "balance": "0.00",
//       "currency": "USD"
//     }
//   ],
//   "wire": [
//     {
//       "accountName": "Uphold Europe Limited",
//       "address": {
//         "line1": "Tartu mnt 2",
//         "line2": "10145 Tallinn, Estonia"
//       },
//       "bic": "LHVBEE22",
//       "currency": "EUR",
//       "iban": "EE76 7700 7710 0159 0178",
//       "name": "AS LHV Pank"
//     },
//     {
//       "accountName": "Uphold HQ, Inc.",
//       "accountNumber": "XXXXXXXXXX",
//       "address": {
//         "line1": "1359 Broadway",
//         "line2": "New York, NY 10018"
//       },
//       "bic": "MCBEUS33",
//       "currency": "USD",
//       "name": "Metropolitan Bank",
//       "routingNumber": "XXXXXXXXX"
//     }
//   ]
// }

ledger::Result ParseFetchUpholdBalance(
    const ledger::UrlResponse& response,
    double* available) {
  DCHECK(available);

  if (response.status_code == net::HTTP_UNAUTHORIZED ||
      response.status_code == net::HTTP_NOT_FOUND ||
      response.status_code == net::HTTP_FORBIDDEN) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* available_str = dictionary->FindStringKey("available");
  if (!available_str) {
    BLOG(0, "Missing available");
    return ledger::Result::LEDGER_ERROR;
  }

  const bool success = base::StringToDouble(*available_str, available);
  if (!success) {
    *available = 0.0;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST https://api.uphold.com/oauth2/token
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "access_token": "edc8b465fe2e2a26ce553d937ccc6c7195e9f909",
//   "token_type": "bearer",
//   "expires_in": 7775999,
//   "scope": "accounts:read accounts:write cards:read cards:write user:read"
// }

ledger::Result ParseUpholdAuthorization(
    const ledger::UrlResponse& response,
    std::string* token) {
  DCHECK(token);

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* access_token = dictionary->FindStringKey("access_token");
  if (!access_token) {
    BLOG(0, "Missing access token");
    return ledger::Result::LEDGER_ERROR;
  }

  *token = *access_token;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET https://api.uphold.com/v0/me
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "address": {
//     "city": "Anytown",
//     "line1": "123 Main Street",
//     "zipCode": "12345"
//   },
//   "birthdate": "1971-06-22",
//   "country": "US",
//   "email": "erogul@example.com",
//   "firstName": "Emerick",
//   "fullName": "Emerick Rogul",
//   "id": "b34060c9-5ca3-4bdb-bc32-1f826ecea36e",
//   "identityCountry": "US",
//   "lastName": "Rogul",
//   "name": "Emerick Rogul",
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

ledger::Result ParseUpholdGetUser(
    const ledger::UrlResponse& response,
    braveledger_uphold::User* user) {
  DCHECK(user);

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* name = dictionary->FindStringKey("firstName");
  if (name) {
    user->name = *name;
  }

  const auto* member_at = dictionary->FindStringKey("memberAt");
  if (member_at) {
    user->member_at = *member_at;
    user->verified = !user->member_at.empty();
  }

  const auto* currencies = dictionary->FindListKey("currencies");
  if (currencies) {
    const std::string currency = "BAT";
    auto bat_in_list = std::find(
        currencies->GetList().begin(),
        currencies->GetList().end(),
        base::Value(currency));
    user->bat_not_allowed = bat_in_list == currencies->GetList().end();
  }

  const auto* status = dictionary->FindStringKey("status");
  if (status) {
    user->status = GetUserStatus(*status);
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET https://api.uphold.com/v0/me/cards/{wallet_address}/addresses
//
// Success:
// OK (200)
//
// Response Format:
// [
//   {
//     "formats": [
//       {
//         "format": "uuid",
//         "value": "d3f67620-abda-4a6f-8d60-b16914341688"
//       }
//     ],
//     "type": "anonymous"
//   }
// ]

ledger::Result ParseUpholdGetCardAddresses(
    const ledger::UrlResponse& response,
    std::map<std::string, std::string>* addresses) {
  DCHECK(addresses);

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> dictionary =
      base::JSONReader::Read(response.body);
  if (!dictionary || !dictionary->is_list()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::ListValue* addresses_list = nullptr;
  if (!dictionary->GetAsList(&addresses_list)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  for (auto& address_item : *addresses_list) {
    base::DictionaryValue* address = nullptr;
    if (!address_item.GetAsDictionary(&address)) {
      continue;
    }

    const auto* type_key = address->FindStringKey("type");
    if (!type_key) {
      continue;
    }
    const std::string type = *type_key;

    auto* formats = address->FindListKey("formats");
    if (!formats) {
      continue;
    }

    if (formats->GetList().size() == 0) {
      continue;
    }

    base::DictionaryValue* format = nullptr;
    if (!formats->GetList()[0].GetAsDictionary(&format)) {
      continue;
    }

    const auto* address_value = format->FindStringKey("value");
    if (!address_value) {
      continue;
    }

    addresses->insert(std::make_pair(type, *address_value));
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// PATCH https://api.uphold.com/v0/me/cards/{wallet_address}
// POST https://api.uphold.com/v0/me/cards/{wallet_address}/addresses
//
// Success:
// OK (200)
//
// Response Format (success):
// {
//   "id": "d3f67620-abda-4a6f-8d60-b16914341688",
//   "network": "anonymous"
// }
//
// Response Format (error):
// {
//   "code": "validation_failed",
//   "errors": {
//     "network": [
//       {
//         "code": "invalid",
//         "message": "This value is not valid"
//       }
//     ]
//   }
// }

ledger::Result ParseUpholdCreateCard(
    const ledger::UrlResponse& response,
    std::string* id) {
  DCHECK(id);

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* id_str = dictionary->FindStringKey("id");
  if (!id_str) {
    BLOG(0, "Missing id");
    return ledger::Result::LEDGER_ERROR;
  }

  *id = *id_str;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET https://api.uphold.com/v0/me/cards?q=currency:BAT
//
// Success:
// OK (200)
//
// Response Format:
// [
//   {
//     "CreatedByApplicationId": null,
//     "address": {
//       "wire": "XXXXXXXXXX"
//     },
//     "available": "12.35",
//     "balance": "12.35",
//     "currency": "BAT",
//     "id": "3ed3b2c4-a715-4c01-b302-fa2681a971ea",
//     "label": "Twitter - Emerick Rogul - Brave Rewards",
//     "lastTransactionAt": "2020-03-31T19:27:57.552Z",
//     "settings": {
//       "position": 7,
//       "protected": false,
//       "starred": true
//     },
//     "normalized": [
//       {
//         "available": "3.15",
//         "balance": "3.15",
//         "currency": "USD"
//       }
//     ],
//     "wire": [
//       {
//         "accountName": "Uphold Europe Limited",
//         "address": {
//           "line1": "Tartu mnt 2",
//           "line2": "10145 Tallinn, Estonia"
//         },
//         "bic": "LHVBEE22",
//         "currency": "EUR",
//         "iban": "EE76 7700 7710 0159 0178",
//         "name": "AS LHV Pank"
//       },
//       {
//         "accountName": "Uphold HQ, Inc.",
//         "accountNumber": "XXXXXXXXXX",
//         "address": {
//           "line1": "1359 Broadway",
//           "line2": "New York, NY 10018"
//         },
//         "bic": "MCBEUS33",
//         "currency": "USD",
//         "name": "Metropolitan Bank",
//         "routingNumber": "XXXXXXXXX"
//       }
//     ]
//   }
// ]

ledger::Result ParseUpholdGetCards(
    const ledger::UrlResponse& response,
    const std::string& card_name,
    std::string* id) {
  DCHECK(id);

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_list()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& it : list->GetList()) {
    const auto* label = it.FindStringKey("label");
    if (!label) {
      continue;
    }

    if (*label == card_name) {
      const auto* id_str = it.FindStringKey("id");
      if (!id_str) {
        continue;
      }

      *id = *id_str;

      return ledger::Result::LEDGER_OK;
    }
  }

  return ledger::Result::LEDGER_ERROR;
}

// Request Url:
// POST https://api.uphold.com/v0/me/cards
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "CreatedByApplicationId": "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
//   "address": {
//     "wire": "XXXXXXXXXX"
//   },
//   "available": "0.00",
//   "balance": "0.00",
//   "currency": "BAT",
//   "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//   "label": "Brave Browser",
//   "lastTransactionAt": null,
//   "settings": {
//     "position": 8,
//     "protected": false,
//     "starred": false
//   },
//   "createdByApplicationClientId": "4c2b665ca060d912fec5c735c734859a06118cc8",
//   "normalized": [
//     {
//       "available": "0.00",
//       "balance": "0.00",
//       "currency": "USD"
//     }
//   ],
//   "wire": [
//     {
//       "accountName": "Uphold Europe Limited",
//       "address": {
//         "line1": "Tartu mnt 2",
//         "line2": "10145 Tallinn, Estonia"
//       },
//       "bic": "LHVBEE22",
//       "currency": "EUR",
//       "iban": "EE76 7700 7710 0159 0178",
//       "name": "AS LHV Pank"
//     },
//     {
//       "accountName": "Uphold HQ, Inc.",
//       "accountNumber": "XXXXXXXXXX",
//       "address": {
//         "line1": "1359 Broadway",
//         "line2": "New York, NY 10018"
//       },
//       "bic": "MCBEUS33",
//       "currency": "USD",
//       "name": "Metropolitan Bank",
//       "routingNumber": "XXXXXXXXX"
//     }
//   ]
// }

ledger::Result ParseUpholdGetCard(
    const ledger::UrlResponse& response,
    std::string* id) {
  DCHECK(id);

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* id_str = dictionary->FindStringKey("id");
  if (!id_str) {
    BLOG(0, "Missing id");
    return ledger::Result::LEDGER_ERROR;
  }

  *id = *id_str;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST https://api.uphold.com/v0/me/cards/{wallet_address}/transactions
//
// Success:
// Accepted (202)
//
// Response Format:
// {
//   "createdAt": "2020-06-10T18:58:21.683Z",
//   "denomination": {
//     "amount": "1.00",
//     "currency": "BAT",
//     "pair": "BATBAT",
//     "rate": "1.00"
//   },
//   "fees": [],
//   "id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
//   "network": "uphold",
//   "normalized": [
//     {
//       "commission": "0.00",
//       "currency": "USD",
//       "fee": "0.00",
//       "rate": "0.24688",
//       "target": "origin",
//       "amount": "0.25"
//     }
//   ],
//   "params": {
//     "currency": "BAT",
//     "margin": "0.00",
//     "pair": "BATBAT",
//     "rate": "1.00",
//     "ttl": 3599588,
//     "type": "internal"
//   },
//   "priority": "normal",
//   "status": "pending",
//   "type": "transfer",
//   "destination": {
//     "amount": "1.00",
//     "base": "1.00",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Brave Software International",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "6654ecb0-6079-4f6c-ba58-791cc890a561",
//       "type": "card",
//       "user": {
//         "id": "f5e37294-68f1-49ae-89e2-b24b64aedd37",
//         "username": "braveintl"
//       }
//     },
//     "rate": "1.00",
//     "type": "card",
//     "username": "braveintl"
//   },
//   "origin": {
//     "amount": "1.00",
//     "base": "1.00",
//     "CardId": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Emerick Rogul",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//       "type": "card",
//       "user": {
//         "id": "b34060c9-5ca3-4bdb-bc32-1f826ecea36e"
//       }
//     },
//     "rate": "1.00",
//     "sources": [],
//     "type": "card"
//   }
// }

ledger::Result ParseUpholdCreateTransaction(
    const ledger::UrlResponse& response,
    std::string* id) {
  DCHECK(id);

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_ACCEPTED) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* id_str = dictionary->FindStringKey("id");
  if (!id_str) {
    BLOG(0, "Missing id");
    return ledger::Result::LEDGER_ERROR;
  }

  *id = *id_str;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST https://api.uphold.com/v0/me/cards/{wallet_address}/transactions/{transaction_id}/commit
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "application": {
//     "name": "Brave Browser"
//   },
//   "createdAt": "2020-06-10T18:58:22.351Z",
//   "denomination": {
//     "pair": "BATBAT",
//     "rate": "1.00",
//     "amount": "1.00",
//     "currency": "BAT"
//   },
//   "fees": [],
//   "id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
//   "message": null,
//   "network": "uphold",
//   "normalized": [
//     {
//       "fee": "0.00",
//       "rate": "0.24688",
//       "amount": "0.25",
//       "target": "origin",
//       "currency": "USD",
//       "commission": "0.00"
//     }
//   ],
//   "params": {
//     "currency": "BAT",
//     "margin": "0.00",
//     "pair": "BATBAT",
//     "progress": "1",
//     "rate": "1.00",
//     "ttl": 3599588,
//     "type": "internal"
//   },
//   "priority": "normal",
//   "reference": null,
//   "status": "completed",
//   "type": "transfer",
//   "destination": {
//     "amount": "1.00",
//     "base": "1.00",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Brave Software International",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "6654ecb0-6079-4f6c-ba58-791cc890a561",
//       "type": "card",
//       "user": {
//         "id": "f5e37294-68f1-49ae-89e2-b24b64aedd37",
//         "username": "braveintl"
//       }
//     },
//     "rate": "1.00",
//     "type": "card",
//     "username": "braveintl"
//   },
//   "origin": {
//     "amount": "1.00",
//     "base": "1.00",
//     "CardId": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Emerick Rogul",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//       "type": "card",
//       "user": {
//         "id": "b34060c9-5ca3-4bdb-bc32-1f826ecea36e"
//       }
//     },
//     "rate": "1.00",
//     "sources": [
//       {
//         "id": "463dca02-83ec-4bd6-93b0-73bf5dbe35ac",
//         "amount": "1.00"
//       }
//     ],
//     "type": "card"
//   }
// }

ledger::Result CheckUpholdCommitTransaction(
    const ledger::UrlResponse& response) {
  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

}  // namespace braveledger_response_util
