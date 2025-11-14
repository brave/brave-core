/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/near_verifier.h"

#include <utility>

#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

namespace {

constexpr char kVerificationPathPrefix[] = "v1/near-result-verification/";
constexpr base::TimeDelta kPendingRetryInterval = base::Seconds(2);
constexpr base::TimeDelta kServerErrorRetryInterval = base::Seconds(10);
constexpr base::TimeDelta kMaxPendingTime = base::Minutes(1);
constexpr char kStatusKey[] = "status";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat_near_verification", R"(
      semantics {
        sender: "AI Chat NEAR Verification"
        description:
          "This is used to verify NEAR AI responses by checking "
          "the attestation status of completions."
        trigger:
          "Triggered when an AI Chat response is received from a NEAR-based "
          "model that requires verification."
        data:
          "Log IDs from AI model completions. No user data is sent."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

NEARVerifier::VerificationState::VerificationState() = default;
NEARVerifier::VerificationState::~VerificationState() = default;

NEARVerifier::NEARVerifier(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    ModelService* model_service,
    VerificationCompletionCallback completion_callback)
    : url_loader_factory_(url_loader_factory),
      model_service_(model_service),
      completion_callback_(std::move(completion_callback)) {}

NEARVerifier::~NEARVerifier() = default;

void NEARVerifier::MaybeVerifyConversationEntry(
    const mojom::ConversationTurn& turn) {
  if (!turn.uuid.has_value() || !turn.model_key.has_value() ||
      !turn.events.has_value() || turn.events->empty()) {
    return;
  }

  const auto* model = model_service_->GetModel(*turn.model_key);
  if (!model || !model->is_near_model) {
    return;
  }

  if (!api_request_helper_) {
    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            GetNetworkTrafficAnnotationTag(), url_loader_factory_);
  }

  const std::string& model_name = model->options->get_leo_model_options()->name;
  const std::string& turn_uuid = turn.uuid.value();

  auto state = std::make_unique<VerificationState>();
  state->turn_uuid = turn_uuid;
  state->model_name = model_name;
  state->start_time = base::TimeTicks::Now();

  base::flat_set<std::string> known_log_ids;

  for (const auto& event : turn.events.value()) {
    if (!event->is_completion_event()) {
      continue;
    }

    const auto& completion_event = event->get_completion_event();
    if (!completion_event->log_id.has_value()) {
      continue;
    }

    if (!known_log_ids.insert(*completion_event->log_id).second) {
      // Duplicate log ID, skip
      continue;
    }

    state->pending_requests++;
    VerifyLogId(state.get(), *completion_event->log_id);
  }

  if (state->pending_requests == 0) {
    return;
  }

  verification_states_[turn_uuid] = std::move(state);
}

void NEARVerifier::VerifyLogId(VerificationState* state,
                               const std::string& log_id) {
  auto path =
      base::StrCat({kVerificationPathPrefix, state->model_name, "/", log_id});
  auto url = GetEndpointUrl(false, path);

  auto callback =
      base::BindOnce(&NEARVerifier::OnVerificationResponse,
                     base::Unretained(this), state->turn_uuid, log_id);

  api_request_helper_->Request("GET", url, "", "application/json",
                               std::move(callback));
}

void NEARVerifier::OnVerificationResponse(
    const std::string& turn_uuid,
    const std::string& log_id,
    api_request_helper::APIRequestResult result) {
  auto* state = base::FindPtrOrNull(verification_states_, turn_uuid);
  if (!state) {
    return;
  }

  if (base::TimeTicks::Now() - state->start_time > kMaxPendingTime) {
    // Max pending time reached, end process
    CompleteVerification(turn_uuid, false);
    return;
  }

  if (!result.Is2XXResponseCode()) {
    if (result.response_code() >= 500 && result.response_code() < 600) {
      // Probably a transient error, retry later
      ScheduleRetry(state, log_id, kServerErrorRetryInterval);
      return;
    }
    // Probably a permanent error, end process here
    CompleteVerification(turn_uuid, false);
    return;
  }

  const auto* dict = result.value_body().GetIfDict();
  if (!dict) {
    // Probably a malformed response, retrying won't help, end process here
    CompleteVerification(turn_uuid, false);
    return;
  }

  const auto* status_value = dict->Find(kStatusKey);
  if (!status_value || (!status_value->is_none() && !status_value->is_bool())) {
    // Also a malformed response
    CompleteVerification(turn_uuid, false);
    return;
  }

  if (const auto bool_value = status_value->GetIfBool()) {
    if (!*bool_value) {
      // Verification failed, end process here
      CompleteVerification(turn_uuid, false);
      return;
    }
  } else {
    // Status is still pending, retry later
    ScheduleRetry(state, log_id, kPendingRetryInterval);
    return;
  }

  state->pending_requests--;
  if (state->pending_requests == 0) {
    // This is the last request to be verified successfully,
    // so the verification is complete and successful
    CompleteVerification(turn_uuid, true);
  }
}

void NEARVerifier::ScheduleRetry(VerificationState* state,
                                 const std::string& log_id,
                                 base::TimeDelta interval) {
  state->retry_timers[log_id] = std::make_unique<base::OneShotTimer>();
  state->retry_timers[log_id]->Start(
      FROM_HERE, interval,
      base::BindOnce(&NEARVerifier::VerifyLogId, base::Unretained(this),
                     base::Unretained(state), log_id));
}

void NEARVerifier::CompleteVerification(const std::string& turn_uuid,
                                        bool verified) {
  completion_callback_.Run(turn_uuid, verified);
  verification_states_.erase(turn_uuid);
}

}  // namespace ai_chat
