/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_SERVER_HEALTH_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_SERVER_HEALTH_H_

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom.h"
#include "net/base/backoff_entry.h"

namespace ai_chat {

// How long after listing a server's tools fails they're listed again, in the
// background, while the server is in use. Each failure of those doubles it, up
// to kMCPMaxToolsRetryDelay.
inline constexpr base::TimeDelta kMCPToolsRetryDelay = base::Minutes(1);
inline constexpr base::TimeDelta kMCPMaxToolsRetryDelay = base::Minutes(15);
// The page can ask to reconnect as often as it likes, and each time is a
// request to the server. A reconnect joins a listing under way, so they're
// never made at once, and one this soon after the last attempt is ignored, so
// that a page that asks in a loop can't make them one after another.
inline constexpr base::TimeDelta kMCPMinReconnectInterval = base::Seconds(1);

// What a server's status is after listing its tools failed with `code`.
mojom::MCPServerStatus GetMCPServerStatusForError(MCPErrorCode code);

// Whether one client can list its server's tools, as found by listing them,
// and when to list them again once that fails.
class MCPServerHealth {
 public:
  // `retry` runs when a failing server's tools should be listed again.
  explicit MCPServerHealth(base::RepeatingClosure retry);
  MCPServerHealth(const MCPServerHealth&) = delete;
  MCPServerHealth& operator=(const MCPServerHealth&) = delete;
  ~MCPServerHealth();

  mojom::MCPServerStatus status() const { return status_; }
  // Whether listing failed and hasn't worked since, so callers shouldn't wait
  // for the tools.
  bool IsFailing() const;
  bool is_listing() const { return is_listing_; }

  void OnListingStarted();
  void OnListingSucceeded();
  void OnListingFailed(MCPErrorCode code);
  // Callers stopped waiting for the listing under way, which still decides
  // the status once it ends.
  void OnListingLate();

  // Whether reconnecting now isn't too soon after the last attempt.
  bool CanReconnect() const;
  // Forgets earlier failures, until the next listing ends.
  void StartChecking();

  // Retries are only made while the server is in use.
  void SetInUse(bool in_use);
  bool is_retry_scheduled() const { return retry_timer_.IsRunning(); }

 private:
  void UpdateRetryTimer();

  mojom::MCPServerStatus status_ = mojom::MCPServerStatus::kAvailable;
  net::BackoffEntry backoff_;
  bool is_listing_ = false;
  bool in_use_ = false;
  base::TimeTicks last_attempt_time_;
  base::OneShotTimer retry_timer_;
  base::RepeatingClosure retry_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_SERVER_HEALTH_H_
