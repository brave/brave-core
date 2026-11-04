// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/mcp_server_associated_content.h"

#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "brave/components/ai_chat/core/browser/tools/mcp_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

// Tool permissions are keyed on origin, but servers can share one, e.g. a
// gateway that hosts many authors' servers at different paths, so each server
// gets an origin of its own, made from its id under a reserved TLD so that it
// can't be any site's. An id that isn't a UUID, as the registry makes them,
// gives an opaque origin, against which no permission is recorded.
url::Origin GetPermissionOrigin(std::string_view server_id) {
  const base::Uuid uuid = base::Uuid::ParseLowercase(server_id);
  if (!uuid.is_valid()) {
    return url::Origin();
  }
  return url::Origin::Create(GURL(base::StrCat(
      {url::kHttpsScheme, "://", uuid.AsLowercaseString(), ".mcp.invalid"})));
}

std::optional<std::vector<std::string>> GetToolNames(
    const std::optional<std::vector<MCPToolDefinition>>& tools) {
  if (!tools) {
    return std::nullopt;
  }
  std::vector<std::string> names;
  for (const MCPToolDefinition& tool : *tools) {
    names.push_back(tool.name);
  }
  return names;
}

}  // namespace

MCPServerAssociatedContent::MCPServerAssociatedContent(
    MCPHost& host,
    const mojom::MCPServer& server)
    : host_(host),
      server_(server.Clone()),
      endpoint_origin_(url::Origin::Create(server.endpoint)),
      permission_origin_(GetPermissionOrigin(server.id)),
      client_(host.GetClient(server.id)) {
  set_uuid(base::Uuid::GenerateRandomV4().AsLowercaseString());
  // Only the origin, as a server's path or query can hold a key.
  set_url(endpoint_origin_.GetURL());
  SetTitle(base::UTF8ToUTF16(server.name));
  set_cached_page_content(PageContent("", mojom::ContentType::MCPServer));
  status_ = GetCurrentStatus();
  host_observation_.Observe(&host);
}

MCPServerAssociatedContent::~MCPServerAssociatedContent() {
  // Answered rather than dropped, so that the conversation isn't left waiting.
  for (GetContentToolsCallback& callback : waiting_callbacks_) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  std::vector<std::unique_ptr<Tool>>()));
  }
}

bool MCPServerAssociatedContent::SetMissedLatestAnswer(
    bool missed_latest_answer) {
  return std::exchange(missed_latest_answer_, missed_latest_answer) !=
         missed_latest_answer;
}

void MCPServerAssociatedContent::GetContent(GetPageContentCallback callback) {
  // The value of this content is its tools, not its content.
  std::move(callback).Run(cached_page_content());
}

void MCPServerAssociatedContent::GetContentTools(
    GetContentToolsCallback callback) {
  if (!client_ || IsFailing() || is_listing_late_) {
    std::move(callback).Run(CreateTools());
    return;
  }
  waiting_callbacks_.push_back(std::move(callback));
  if (!is_listing_) {
    StartListing();
  }
}

url::Origin MCPServerAssociatedContent::GetOrigin() const {
  return permission_origin_;
}

void MCPServerAssociatedContent::OnMCPClientsChanged() {
  mojom::MCPServerPtr server;
  base::WeakPtr<MCPClient> client;
  for (MCPHost::EnabledServer& enabled : host_->GetEnabledServers()) {
    if (enabled.server->id == server_->id) {
      server = std::move(enabled.server);
      client = std::move(enabled.client);
      break;
    }
  }
  // It was attached at its origin, which is the one shown, so it isn't used at
  // another.
  if (server && url::Origin::Create(server->endpoint) != endpoint_origin_) {
    client = nullptr;
  }

  bool moved = false;
  if (server) {
    moved = server->endpoint != server_->endpoint;
    const bool renamed = server->name != server_->name;
    server_ = std::move(server);
    if (renamed) {
      SetTitle(base::UTF8ToUTF16(server_->name));
    }
    // Told of as a new page, so that the conversation drops the choices made
    // about its tools, as another endpoint can be another author's server,
    // even at the same origin. Observers are told of its tools there once its
    // client is set.
    if (moved) {
      NotifyNewPage();
    }
  }

  if (client.get() == client_.get()) {
    base::WeakPtr<MCPServerAssociatedContent> self =
        weak_ptr_factory_.GetWeakPtr();
    UpdateStatus();
    // Moved to another origin, it offers no tools.
    if (self && moved) {
      NotifyContentToolsChanged();
    }
    return;
  }
  client_ = std::move(client);
  UpdateServerUse();
  // What the previous client listed, or was listing, no longer applies.
  tools_.reset();
  ++listing_id_;
  is_listing_ = false;
  is_listing_late_ = false;
  base::WeakPtr<MCPServerAssociatedContent> self =
      weak_ptr_factory_.GetWeakPtr();
  // The callers waiting go ahead with the status that this leaves it in.
  UpdateStatus();
  if (!self) {
    return;
  }
  if (client_ && !waiting_callbacks_.empty()) {
    // Listed by the new client, within the time the callers already wait.
    ListTools();
  } else {
    deadline_timer_.Stop();
    RunWaitingCallbacks();
  }
  if (self) {
    NotifyContentToolsChanged();
  }
}

void MCPServerAssociatedContent::OnMCPServerStatusChanged(
    const std::string& server_id) {
  if (server_id != server_->id) {
    return;
  }
  const bool was_available = status_ == mojom::MCPServerStatus::kAvailable;
  base::WeakPtr<MCPServerAssociatedContent> self =
      weak_ptr_factory_.GetWeakPtr();
  UpdateStatus();
  // Tools that were listed in the background are shown, and added to the loop
  // that went without them. A listing of its own does that as it's answered.
  if (self && !was_available && status_ == mojom::MCPServerStatus::kAvailable &&
      client_ && !is_listing_) {
    NotifyContentToolsChanged();
  }
}

void MCPServerAssociatedContent::StartListing() {
  deadline_timer_.Start(FROM_HERE, kMCPToolsDeadline,
                        base::BindOnce(&MCPServerAssociatedContent::OnDeadline,
                                       base::Unretained(this)));
  ListTools();
}

void MCPServerAssociatedContent::ListTools() {
  is_listing_ = true;
  // The client drops its callbacks when it's destroyed, as it is when the
  // server's settings change.
  host_->GetTools(server_->id,
                  mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                      base::BindPostTaskToCurrentDefault(base::BindOnce(
                          &MCPServerAssociatedContent::OnToolsListed,
                          weak_ptr_factory_.GetWeakPtr(), ++listing_id_)),
                      MCPClient::ToolsResult(base::unexpected(MCPError{
                          MCPErrorCode::kNetworkError,
                          base::StrCat({server_->name,
                                        " was changed or removed before it "
                                        "answered."})}))));
}

void MCPServerAssociatedContent::OnToolsListed(int listing_id,
                                               MCPClient::ToolsResult result) {
  if (listing_id != listing_id_) {
    return;
  }
  is_listing_ = false;
  deadline_timer_.Stop();
  const bool was_late = std::exchange(is_listing_late_, false);
  const std::optional<std::vector<std::string>> previous_names =
      GetToolNames(tools_);
  if (result.has_value()) {
    tools_ = std::move(*result);
  } else {
    // The error names the server, and has nothing secret in it.
    DVLOG(1) << result.error().message;
  }
  // Callers were given the previous tools if they stopped waiting, or didn't
  // wait, and observers have been told of none if these are the first.
  const bool tools_changed =
      result.has_value() &&
      (was_late || !previous_names || previous_names != GetToolNames(tools_));

  base::WeakPtr<MCPServerAssociatedContent> self =
      weak_ptr_factory_.GetWeakPtr();
  RunWaitingCallbacks();
  if (self && tools_changed) {
    NotifyContentToolsChanged();
  }
}

void MCPServerAssociatedContent::OnDeadline() {
  is_listing_late_ = true;
  // The callers waiting go ahead knowing that it isn't responding.
  base::WeakPtr<MCPServerAssociatedContent> self =
      weak_ptr_factory_.GetWeakPtr();
  host_->MarkNotResponding(server_->id);
  if (self) {
    RunWaitingCallbacks();
  }
}

void MCPServerAssociatedContent::RunWaitingCallbacks() {
  std::vector<GetContentToolsCallback> callbacks =
      std::exchange(waiting_callbacks_, {});
  base::WeakPtr<MCPServerAssociatedContent> self =
      weak_ptr_factory_.GetWeakPtr();
  for (GetContentToolsCallback& callback : callbacks) {
    std::move(callback).Run(self ? CreateTools()
                                 : std::vector<std::unique_ptr<Tool>>());
  }
}

std::vector<std::unique_ptr<Tool>> MCPServerAssociatedContent::CreateTools()
    const {
  std::vector<std::unique_ptr<Tool>> tools;
  if (!client_ || !tools_) {
    return tools;
  }
  for (std::unique_ptr<MCPTool>& tool :
       MCPTool::CreateAll(*server_, *tools_, client_)) {
    tools.push_back(std::move(tool));
  }
  return tools;
}

bool MCPServerAssociatedContent::IsFailing() const {
  return host_->IsFailing(server_->id);
}

mojom::MCPServerStatus MCPServerAssociatedContent::GetCurrentStatus() const {
  // Moved to another origin, it has a client that isn't used here.
  if (!client_ && host_->GetClient(server_->id)) {
    return mojom::MCPServerStatus::kUnavailable;
  }
  return host_->GetServerStatus(server_->id);
}

void MCPServerAssociatedContent::UpdateStatus() {
  const mojom::MCPServerStatus status = GetCurrentStatus();
  if (status == status_) {
    return;
  }
  status_ = status;
  if (status_ == mojom::MCPServerStatus::kAvailable) {
    missed_latest_answer_ = false;
  }
  NotifyStatusChanged();
}

void MCPServerAssociatedContent::DidChangeToolsAttached() {
  UpdateServerUse();
}

void MCPServerAssociatedContent::UpdateServerUse() {
  if (!client_ || !tools_attached()) {
    server_use_.RunAndReset();
  } else if (!server_use_) {
    server_use_ = host_->UseServer(server_->id);
  }
}

}  // namespace ai_chat
