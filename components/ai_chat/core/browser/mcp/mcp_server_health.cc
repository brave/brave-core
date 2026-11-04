/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_server_health.h"

#include <utility>

#include "base/location.h"
#include "base/notreached.h"

namespace ai_chat {

namespace {

constexpr net::BackoffEntry::Policy kRetryPolicy = {
    .num_errors_to_ignore = 0,
    .initial_delay_ms = kMCPToolsRetryDelay.InMilliseconds(),
    .multiply_factor = 2,
    .jitter_factor = 0,
    .maximum_backoff_ms = kMCPMaxToolsRetryDelay.InMilliseconds(),
    .entry_lifetime_ms = -1,
    .always_use_initial_delay = false,
};

}  // namespace

mojom::MCPServerStatus GetMCPServerStatusForError(MCPErrorCode code) {
  switch (code) {
    // Any of these might pass, so the server may come back.
    case MCPErrorCode::kNetworkError:
    case MCPErrorCode::kTimeout:
    case MCPErrorCode::kNoResponse:
    case MCPErrorCode::kSessionExpired:
    case MCPErrorCode::kServerError:
    case MCPErrorCode::kHttpError:
      return mojom::MCPServerStatus::kNotResponding;
    case MCPErrorCode::kAuthRequired:
    case MCPErrorCode::kAuthRejected:
    case MCPErrorCode::kInvalidToken:
      return mojom::MCPServerStatus::kAuthRequired;
    case MCPErrorCode::kInvalidEndpoint:
    case MCPErrorCode::kInvalidRequest:
    case MCPErrorCode::kResponseTooLarge:
    case MCPErrorCode::kCrossOriginRedirect:
    case MCPErrorCode::kStreamableHttpNotSupported:
    case MCPErrorCode::kUnsupportedProtocolVersion:
    case MCPErrorCode::kToolsNotSupported:
    case MCPErrorCode::kInvalidResponse:
    case MCPErrorCode::kHttpClientError:
      return mojom::MCPServerStatus::kUnsupported;
  }
  NOTREACHED();
}

MCPServerHealth::MCPServerHealth(base::RepeatingClosure retry)
    : backoff_(&kRetryPolicy), retry_(std::move(retry)) {}

MCPServerHealth::~MCPServerHealth() = default;

bool MCPServerHealth::IsFailing() const {
  return status_ != mojom::MCPServerStatus::kAvailable &&
         status_ != mojom::MCPServerStatus::kChecking;
}

void MCPServerHealth::OnListingStarted() {
  is_listing_ = true;
  last_attempt_time_ = base::TimeTicks::Now();
  UpdateRetryTimer();
}

void MCPServerHealth::OnListingSucceeded() {
  is_listing_ = false;
  status_ = mojom::MCPServerStatus::kAvailable;
  backoff_.Reset();
  UpdateRetryTimer();
}

void MCPServerHealth::OnListingFailed(MCPErrorCode code) {
  is_listing_ = false;
  status_ = GetMCPServerStatusForError(code);
  backoff_.InformOfRequest(/*succeeded=*/false);
  UpdateRetryTimer();
}

void MCPServerHealth::OnListingLate() {
  if (is_listing_ && !IsFailing()) {
    status_ = mojom::MCPServerStatus::kNotResponding;
  }
}

bool MCPServerHealth::CanReconnect() const {
  return last_attempt_time_.is_null() ||
         base::TimeTicks::Now() - last_attempt_time_ >=
             kMCPMinReconnectInterval;
}

void MCPServerHealth::StartChecking() {
  status_ = mojom::MCPServerStatus::kChecking;
  backoff_.Reset();
  UpdateRetryTimer();
}

void MCPServerHealth::SetInUse(bool in_use) {
  in_use_ = in_use;
  UpdateRetryTimer();
}

void MCPServerHealth::UpdateRetryTimer() {
  // A listing under way is retried, if need be, once it ends.
  if (!IsFailing() || !in_use_ || is_listing_) {
    retry_timer_.Stop();
    return;
  }
  // One that came due while the server wasn't in use runs straight away.
  if (!retry_timer_.IsRunning()) {
    retry_timer_.Start(FROM_HERE, backoff_.GetTimeUntilRelease(), retry_);
  }
}

}  // namespace ai_chat
