/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_NEAR_VERIFIER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_NEAR_VERIFIER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class NEARVerifier {
 public:
  using GetModelCallback =
      base::RepeatingCallback<const mojom::Model*(std::string_view model_key)>;
  using VerificationCompletionCallback =
      base::RepeatingCallback<void(const std::string& turn_uuid,
                                   bool verified)>;

  NEARVerifier(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      GetModelCallback get_model_callback,
      VerificationCompletionCallback completion_callback);

  NEARVerifier(const NEARVerifier&) = delete;
  NEARVerifier& operator=(const NEARVerifier&) = delete;
  ~NEARVerifier();

  void MaybeVerifyConversationEntry(const mojom::ConversationTurn& turn);

 private:
  struct VerificationState {
    VerificationState();
    ~VerificationState();

    std::string turn_uuid;
    std::string model_name;
    base::TimeTicks start_time;
    base::flat_map<std::string, std::unique_ptr<base::OneShotTimer>>
        retry_timers;
    size_t pending_requests = 0;
  };

  void VerifyLogId(VerificationState* state, const std::string& log_id);

  void OnVerificationResponse(const std::string& turn_uuid,
                              const std::string& log_id,
                              api_request_helper::APIRequestResult result);

  void ScheduleRetry(VerificationState* state,
                     const std::string& log_id,
                     base::TimeDelta interval);

  void CompleteVerification(const std::string& turn_uuid, bool verified);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  GetModelCallback get_model_callback_;
  VerificationCompletionCallback completion_callback_;

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::flat_map<std::string, std::unique_ptr<VerificationState>>
      verification_states_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_NEAR_VERIFIER_H_
