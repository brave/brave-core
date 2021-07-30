/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_RECIPIENT_ID_POST_RECIPIENT_ID_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_RECIPIENT_ID_POST_RECIPIENT_ID_GEMINI_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST https://api.gemini.com/v1/payments/recipientIds
// Payload:
// {
//    "label": <uuid>
// }
//
// Headers:
//   Authorization: Bearer <token>
//   X-GEMINI-PAYLOAD: base64-payload
//
// Request body:
// {}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
// HTTP_NOT_FOUND (404)
//
// Response body:
// {
//    "result": "OK",
//    "recipient_id": "60f9be89-ada7-486d-9cef-f6d3a10886d7",
//    "label": <uuid>
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace gemini {

using PostRecipientIdCallback =
    std::function<void(const type::Result result,
                       const std::string& recipient_id)>;

class PostRecipientId {
 public:
  explicit PostRecipientId(LedgerImpl* ledger);
  ~PostRecipientId();

  void Request(const std::string& token, PostRecipientIdCallback callback);

 private:
  std::string GetUrl();

  type::Result ParseBody(const std::string& body, std::string* recipient_id);

  void OnRequest(const type::UrlResponse& response,
                 PostRecipientIdCallback callback);
  std::string GeneratePayload();

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_RECIPIENT_ID_POST_RECIPIENT_ID_GEMINI_H_
