// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_SERVER_ASSOCIATED_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_SERVER_ASSOCIATED_CONTENT_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_host.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom.h"
#include "url/origin.h"

namespace ai_chat {

class Tool;

// How long a server's tools are waited for, as every message waits for them.
// Long enough for a warm connection's `initialize` and `tools/list`. Tools
// listed later are added to the rest of the generation loop.
inline constexpr base::TimeDelta kMCPToolsDeadline = base::Seconds(5);

// One of the user's MCP servers, attached to a conversation. Like a workspace,
// its value is its tools, so it has no content of its own. Attachments are
// live only: they're never persisted.
//
// The tools are listed whenever they're asked for, which the client serves
// from its cache until that expires. Callers get the tools last listed if the
// server doesn't answer within kMCPToolsDeadline, or while the host has it
// failing. If the server is turned off, removed, or moved to another origin,
// it stays attached but offers no tools until it's usable again. Moved to
// another endpoint, the choices made about its tools are reset.
class MCPServerAssociatedContent : public AssociatedContentDelegate,
                                   public MCPHost::Observer {
 public:
  MCPServerAssociatedContent(MCPHost& host, const mojom::MCPServer& server);
  MCPServerAssociatedContent(const MCPServerAssociatedContent&) = delete;
  MCPServerAssociatedContent& operator=(const MCPServerAssociatedContent&) =
      delete;
  ~MCPServerAssociatedContent() override;

  const std::string& server_id() const { return server_->id; }

  // The host's status for the server, unless it's moved to another origin,
  // where it isn't used. Observers are told when it changes, and
  // missed_latest_answer() with it.
  mojom::MCPServerStatus status() const { return status_; }

  // Whether the latest answer was written without any of the server's tools,
  // as it wasn't available. Cleared once it is.
  bool missed_latest_answer() const { return missed_latest_answer_; }
  // Returns whether it changed. Observers aren't told.
  bool SetMissedLatestAnswer(bool missed_latest_answer);

  // AssociatedContentDelegate:
  void GetContent(GetPageContentCallback callback) override;
  // Tells observers when tools are first listed, when they change, and when
  // they arrive after callers stopped waiting.
  void GetContentTools(GetContentToolsCallback callback) override;
  url::Origin GetOrigin() const override;

 protected:
  // AssociatedContentDelegate:
  void DidChangeToolsAttached() override;

 private:
  // MCPHost::Observer:
  void OnMCPClientsChanged() override;
  void OnMCPServerStatusChanged(const std::string& server_id) override;

  // Lists the tools, and stops waiting for them after kMCPToolsDeadline.
  void StartListing();
  void ListTools();
  // Ignores the answer to a listing that's been abandoned.
  void OnToolsListed(int listing_id, MCPClient::ToolsResult result);
  void OnDeadline();
  // Gives the callers waiting the tools last listed.
  void RunWaitingCallbacks();
  std::vector<std::unique_ptr<Tool>> CreateTools() const;
  bool IsFailing() const;
  mojom::MCPServerStatus GetCurrentStatus() const;
  // Brings status() up to date, and tells observers if it changed.
  void UpdateStatus();
  // Holds a use of the server while its tools are offered here.
  void UpdateServerUse();

  const raw_ref<MCPHost> host_;
  mojom::MCPServerPtr server_;
  // Where the server was when it was attached.
  const url::Origin endpoint_origin_;
  // What tool permissions are keyed on: the server's own, as servers can share
  // an endpoint origin.
  const url::Origin permission_origin_;
  // Null while the server can't be used.
  base::WeakPtr<MCPClient> client_;
  // Held while the server's tools are offered here, so that the host retries
  // it while it's failing.
  base::ScopedClosureRunner server_use_;
  mojom::MCPServerStatus status_ = mojom::MCPServerStatus::kAvailable;
  bool missed_latest_answer_ = false;

  // The tools last listed, unset until they first are.
  std::optional<std::vector<MCPToolDefinition>> tools_;
  // Identifies the listing in progress, so that the answer to one that's been
  // abandoned is ignored.
  int listing_id_ = 0;
  bool is_listing_ = false;
  // Whether callers stopped waiting for the listing in progress.
  bool is_listing_late_ = false;
  std::vector<GetContentToolsCallback> waiting_callbacks_;
  base::OneShotTimer deadline_timer_;

  base::ScopedObservation<MCPHost, MCPHost::Observer> host_observation_{this};
  base::WeakPtrFactory<MCPServerAssociatedContent> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_SERVER_ASSOCIATED_CONTENT_H_
