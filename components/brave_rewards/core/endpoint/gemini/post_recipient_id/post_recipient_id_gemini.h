/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_POST_RECIPIENT_ID_POST_RECIPIENT_ID_GEMINI_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_POST_RECIPIENT_ID_POST_RECIPIENT_ID_GEMINI_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

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

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint::gemini {

using PostRecipientIdCallback =
    base::OnceCallback<void(mojom::Result, std::string&& recipient_id)>;

class PostRecipientId {
 public:
  static inline const std::string kRecipientLabel = "Brave Browser";

  explicit PostRecipientId(RewardsEngineImpl& engine);
  ~PostRecipientId();

  void Request(const std::string& token, PostRecipientIdCallback);

 private:
  std::string GetUrl();

  mojom::Result ParseBody(const std::string& body, std::string* recipient_id);

  void OnRequest(PostRecipientIdCallback, mojom::UrlResponsePtr);
  std::string GeneratePayload();

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace endpoint::gemini
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_POST_RECIPIENT_ID_POST_RECIPIENT_ID_GEMINI_H_
