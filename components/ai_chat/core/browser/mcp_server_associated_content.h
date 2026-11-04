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

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_host.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom.h"
#include "url/origin.h"

namespace ai_chat {

class MCPClient;
class Tool;

// How long a server's tools are waited for, as every message waits for them.
// Long enough for a warm connection's `initialize` and `tools/list`. Tools
// listed later are added to the rest of the generation loop.
inline constexpr base::TimeDelta kMCPToolsDeadline = base::Seconds(5);
// How long a server isn't asked for its tools after it fails to list them, so
// that one that's down doesn't hold up each message.
inline constexpr base::TimeDelta kMCPToolsRetryDelay = base::Minutes(1);

// One of the user's MCP servers, attached to a conversation. Like a workspace,
// its value is its tools, so it has no content of its own. Attachments are
// live only: they're never persisted.
//
// The tools are listed whenever they're asked for, which the client serves
// from its cache until that expires. Callers get the tools last listed if the
// server doesn't answer within kMCPToolsDeadline, or while it's backing off
// after a failure. If the server is turned off, removed, or moved to another
// origin, it stays attached but offers no tools until it's usable again.
// Moved to another endpoint, the choices made about its tools are reset.
class MCPServerAssociatedContent : public AssociatedContentDelegate,
                                   public MCPHost::Observer {
 public:
  MCPServerAssociatedContent(MCPHost& host, const mojom::MCPServer& server);
  MCPServerAssociatedContent(const MCPServerAssociatedContent&) = delete;
  MCPServerAssociatedContent& operator=(const MCPServerAssociatedContent&) =
      delete;
  ~MCPServerAssociatedContent() override;

  const std::string& server_id() const { return server_->id; }

  // AssociatedContentDelegate:
  void GetContent(GetPageContentCallback callback) override;
  // Tells observers when tools are first listed, when they change, and when
  // they arrive after callers stopped waiting.
  void GetContentTools(GetContentToolsCallback callback) override;
  url::Origin GetOrigin() const override;

 private:
  using ToolsResult = base::expected<std::vector<MCPToolDefinition>, MCPError>;

  // MCPHost::Observer:
  void OnMCPClientsChanged() override;

  void StartListing();
  // Ignores the answer to a listing that's been abandoned.
  void OnToolsListed(int listing_id, ToolsResult result);
  void OnDeadline();
  // Gives the callers waiting the tools last listed.
  void RunWaitingCallbacks();
  std::vector<std::unique_ptr<Tool>> CreateTools() const;
  bool IsBackingOff() const;

  const raw_ref<MCPHost> host_;
  mojom::MCPServerPtr server_;
  // Where the server was when it was attached.
  const url::Origin endpoint_origin_;
  // What tool permissions are keyed on: the server's own, as servers can share
  // an endpoint origin.
  const url::Origin permission_origin_;
  // Null while the server can't be used.
  base::WeakPtr<MCPClient> client_;

  // The tools last listed, unset until they first are.
  std::optional<std::vector<MCPToolDefinition>> tools_;
  // When listing last failed, or null.
  base::TimeTicks last_failure_time_;
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
