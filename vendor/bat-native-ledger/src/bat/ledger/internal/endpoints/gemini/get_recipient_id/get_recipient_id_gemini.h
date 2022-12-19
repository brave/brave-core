/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GEMINI_GET_RECIPIENT_ID_GET_RECIPIENT_ID_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GEMINI_GET_RECIPIENT_ID_GET_RECIPIENT_ID_GEMINI_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/request_builder.h"
#include "bat/ledger/internal/endpoints/response_handler.h"
#include "bat/ledger/internal/endpoints/result_for.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// GET /v1/payments/recipientIds
//
// Request body:
// -
//
// Response body:
// [
//   {
//     "label": "95eac685-3e3e-4e5d-a32d-5bc18716cb0d",
//     "recipient_id": "621609a9-ce36-453f-b892-0d7b42212329"
//   }, {
//     "label": "de476441-a834-4b93-82e3-3226e5153f73",
//     "recipient_id": "621d392c-75b3-b655-94e4-2849a44d38a9"
//   }, {
//     "label": "Brave Browser",
//     "recipient_id": "6378fc55-18db-488a-85a3-1af557767d0a"
//   }
// ]

namespace ledger {
class LedgerImpl;

namespace endpoints {

class GetRecipientIDGemini;

template <>
struct ResultFor<GetRecipientIDGemini> {
  using Value = std::string;  // recipient ID
  using Error = mojom::GetRecipientIDGeminiError;
};

class GetRecipientIDGemini final
    : public RequestBuilder,
      public ResponseHandler<GetRecipientIDGemini> {
 public:
  static Result ProcessResponse(const mojom::UrlResponse&);

  GetRecipientIDGemini(LedgerImpl*, std::string&& token);
  ~GetRecipientIDGemini() override;

 private:
  absl::optional<std::string> Url() const override;
  mojom::UrlMethod Method() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;

  std::string token_;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GEMINI_GET_RECIPIENT_ID_GET_RECIPIENT_ID_GEMINI_H_
