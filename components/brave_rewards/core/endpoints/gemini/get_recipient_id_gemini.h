/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_GEMINI_GET_RECIPIENT_ID_GEMINI_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_GEMINI_GET_RECIPIENT_ID_GEMINI_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"

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

namespace brave_rewards::internal {
class RewardsEngineImpl;

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
  static Result ProcessResponse(RewardsEngineImpl& engine,
                                const mojom::UrlResponse&);

  GetRecipientIDGemini(RewardsEngineImpl& engine, std::string&& token);
  ~GetRecipientIDGemini() override;

 private:
  std::optional<std::string> Url() const override;
  mojom::UrlMethod Method() const override;
  std::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;

  std::string token_;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_GEMINI_GET_RECIPIENT_ID_GEMINI_H_
