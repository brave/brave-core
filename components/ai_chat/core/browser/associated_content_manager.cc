// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_manager.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/barrier_closure.h"
#include "base/check.h"
#include "base/containers/map_util.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/mcp_server_associated_content.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

namespace {
constexpr size_t kMaxToolsPerContent = 30;
}  // namespace

AssociatedContentManager::AssociatedContentManager(
    ConversationHandler* conversation)
    : conversation_(conversation) {}

AssociatedContentManager::FetchedContentTools::FetchedContentTools() = default;
AssociatedContentManager::FetchedContentTools::FetchedContentTools(
    FetchedContentTools&&) noexcept = default;
AssociatedContentManager::FetchedContentTools&
AssociatedContentManager::FetchedContentTools::operator=(
    FetchedContentTools&&) noexcept = default;
AssociatedContentManager::FetchedContentTools::~FetchedContentTools() = default;

AssociatedContentManager::~AssociatedContentManager() {
  DVLOG(1) << __func__;

  DetachContent();
}

void AssociatedContentManager::LoadArchivedContent(
    const mojom::Conversation* metadata,
    const mojom::ConversationArchivePtr& archive) {
  DVLOG(2) << __func__
           << " metadata size: " << metadata->associated_content.size()
           << ", archive size: " << archive->associated_content.size();

  // Remove all owned content - it should be reloaded from the DB. MCP servers
  // are never stored, so they stay.
  for (int i = owned_content_.size() - 1; i >= 0; --i) {
    if (IsMCPServer(owned_content_[i].get())) {
      continue;
    }
    RemoveContent(owned_content_[i].get(), /*notify_updated=*/false);
  }

  for (size_t i = 0; i < archive->associated_content.size(); ++i) {
    const auto& archive_content = archive->associated_content[i];
    const auto& content_it = std::ranges::find_if(
        metadata->associated_content, [&archive_content](const auto& content) {
          return archive_content->content_uuid == content->uuid;
        });
    if (content_it == metadata->associated_content.end()) {
      continue;
    }

    auto* content = content_it->get();
    // An attached MCP server lasts only as long as the conversation stays
    // loaded, so there's nothing to restore.
    if (content->content_type == mojom::ContentType::MCPServer) {
      continue;
    }
    std::unique_ptr<AssociatedContentDelegate> owned_content;
    if (content->content_type == mojom::ContentType::Workspace) {
      owned_content = conversation_->ai_chat_service()
                          ->RestoreWorkspaceAssociatedContentFromUrl(
                              content->url, content->uuid);
    }

    // Not a workspace, or workspace restoration failed, so create an archive.
    if (!owned_content) {
      bool is_video =
          (content->content_type == mojom::ContentType::VideoTranscript);
      owned_content = std::make_unique<AssociatedArchiveContent>(
          content->url, archive_content->content,
          base::UTF8ToUTF16(content->title), is_video, content->uuid);
    }

    CHECK(owned_content);
    owned_content_.push_back(std::move(owned_content));
    AddContent(owned_content_.back().get(), /*notify_updated=*/false);

    // Be sure to record the turn that this content is associated with.
    content_uuid_to_conversation_turns_[archive_content->content_uuid] = {
        .turn_uuid = archive_content->conversation_turn_uuid};
  }

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::CreateArchiveContent(
    AssociatedContentDelegate* to_archive) {
  DVLOG(1) << __func__;
  auto content_uuid = to_archive->uuid();
  auto text_content = to_archive->cached_page_content().content;
  auto is_video = to_archive->cached_page_content().content_type ==
                  mojom::ContentType::VideoTranscript;

  auto it = std::ranges::find(content_delegates_, content_uuid,
                              [](const auto& ptr) { return ptr->uuid(); });
  CHECK(it != content_delegates_.end()) << "Couldn't find |content_id|";

  AssociatedContentDelegate* delegate = *it;
  content_observations_.RemoveObservation(delegate);
  RunContentToolsAddedCallbacks(content_uuid, 0);

  // Construct a "content archive" implementation of AssociatedContentDelegate
  // with a duplicate of the article text.
  owned_content_.emplace_back(std::make_unique<AssociatedArchiveContent>(
      delegate->url(), std::move(text_content), delegate->title(), is_video,
      delegate->uuid()));
  *it = owned_content_.back().get();
  content_observations_.AddObservation(*it);

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::AddContent(AssociatedContentDelegate* delegate,
                                          bool notify_updated,
                                          bool detach_existing_content) {
  DVLOG(1) << __func__;

  // Optionally, we can set |delegate| as the only content for this
  // conversation.
  if (detach_existing_content) {
    DetachContent();
  }

  if (delegate) {
    // If we've already added this delegate, don't add it again.
    // Note: We can get here if the user is clicking around quickly in the
    // attachments UI.
    if (std::ranges::find(content_delegates_, delegate, [](const auto& ptr) {
          return ptr;
        }) != content_delegates_.end()) {
      return;
    }

    // Note: When we add a delegate to a conversation we should fetch the
    // content. Otherwise we can end up with a Snapshot with no content (i.e. if
    // the tab is closed).
    // We don't try and keep the content alive to force letting the content to
    // fetch because its a bit of an edge case, and there are no real
    // consequences of not having the content (except for the content not being
    // attached).
    // Additionally, we want to call GetContent even if `notify_updated` is
    // false so we cache the attached content.
    delegate->GetContent(base::BindOnce(
        [](base::WeakPtr<AssociatedContentManager> self, bool notify_updated,
           PageContent content) {
          if (!self || !notify_updated) {
            return;
          }

          // Note: |is_video| may have changed so we need to notify the
          // conversation.
          self->conversation_->OnAssociatedContentUpdated();
        },
        weak_ptr_factory_.GetWeakPtr(), notify_updated));

    content_delegates_.push_back(delegate);
    content_observations_.AddObservation(delegate);

    // Let the content start watching for tool changes now that there's a
    // conversation to report them to.
    delegate->OnAssociatedWithConversation();

    // Discover whether this content exposes tools so it can be attached and
    // surfaced in the tools pill without waiting for a generation loop. An MCP
    // server's tools are attached whatever it offers.
    if (!IsMCPServer(delegate)) {
      DetectContentTools(delegate);
    }
  }

  if (notify_updated) {
    conversation_->OnAssociatedContentUpdated();
  }
}

void AssociatedContentManager::AddOwnedContent(
    std::unique_ptr<AssociatedContentDelegate> delegate,
    bool notify_updated) {
  owned_content_.push_back(std::move(delegate));
  AddContent(owned_content_.back().get(), notify_updated);
}

void AssociatedContentManager::AddMCPServer(
    std::unique_ptr<MCPServerAssociatedContent> server) {
  MCPServerAssociatedContent* delegate = server.get();
  mcp_server_ids_.emplace(delegate->uuid(), delegate->server_id());
  AddOwnedContent(std::move(server), /*notify_updated=*/false);
  // Tells the conversation, and adds the tools to the current loop.
  delegate->set_tools_attached(true);
}

bool AssociatedContentManager::HasMCPServer(std::string_view server_id) const {
  return std::ranges::contains(
      mcp_server_ids_, server_id,
      [](const auto& entry) -> const std::string& { return entry.second; });
}

void AssociatedContentManager::RemoveContent(
    AssociatedContentDelegate* delegate,
    bool notify_updated) {
  DVLOG(1) << __func__;

  auto it = std::ranges::find(content_delegates_, delegate,
                              [](const auto& ptr) { return ptr; });
  url::Origin origin;
  std::vector<std::string> later_mcp_servers;
  if (it != content_delegates_.end()) {
    // Captured now, as erasing owned content below may delete |delegate|.
    origin = delegate->GetOrigin();
    if (IsMCPServer(delegate)) {
      later_mcp_servers = GetMCPServersFrom(delegate, /*inclusive=*/false);
    }
    // Let the content know it isn't associated with this conversation
    // anymore.
    content_observations_.RemoveObservation(delegate);
    content_delegates_.erase(it);
    tools_attachment_overridden_.erase(delegate->uuid());
    mcp_server_ids_.erase(delegate->uuid());
    RunContentToolsAddedCallbacks(delegate->uuid(), 0);
  }

  // If this is owned content, delete it.
  auto owned_it = std::ranges::find_if(
      owned_content_,
      [delegate](const auto& content) { return content.get() == delegate; });
  if (owned_it != owned_content_.end()) {
    owned_content_.erase(owned_it);
  }

  MaybeResetToolPermissionsForOrigin(origin);
  PushMCPServerToolInfos(later_mcp_servers);

  if (notify_updated) {
    conversation_->OnAssociatedContentUpdated();
  }
}

void AssociatedContentManager::RemoveContent(std::string_view content_uuid,
                                             bool notify_updated) {
  DVLOG(1) << __func__;

  auto it = std::ranges::find_if(content_delegates_,
                                 [&content_uuid](const auto& delegate) {
                                   return delegate->uuid() == content_uuid;
                                 });
  if (it != content_delegates_.end()) {
    RemoveContent(*it, notify_updated);
  }
}

void AssociatedContentManager::SetToolsAttached(std::string_view content_uuid,
                                                bool tools_attached) {
  DVLOG(1) << __func__;

  // Record even when it matches the current state, so auto-updates stop.
  tools_attachment_overridden_.insert(std::string(content_uuid));

  auto it = std::ranges::find_if(content_delegates_,
                                 [&content_uuid](const auto& delegate) {
                                   return delegate->uuid() == content_uuid;
                                 });
  if (it == content_delegates_.end() ||
      (*it)->tools_attached() == tools_attached) {
    return;
  }

  (*it)->set_tools_attached(tools_attached);
}

void AssociatedContentManager::GetToolInfos(std::string_view content_uuid,
                                            GetToolInfosCallback callback) {
  auto it = std::ranges::find_if(content_delegates_,
                                 [&content_uuid](const auto& delegate) {
                                   return delegate->uuid() == content_uuid;
                                 });
  if (it == content_delegates_.end()) {
    std::move(callback).Run({});
    return;
  }

  if (!IsMCPServer(*it)) {
    (*it)->GetContentTools(
        base::BindOnce(&AssociatedContentManager::OnToolInfosFetched,
                       weak_ptr_factory_.GetWeakPtr(), (*it)->GetOrigin(),
                       std::move(callback)));
    return;
  }

  // The cap across MCP servers cuts a server's tools after those of the
  // servers attached before it, so they're fetched too.
  std::vector<AssociatedContentDelegate*> servers;
  for (AssociatedContentDelegate* delegate : content_delegates_) {
    if (delegate == *it) {
      servers.push_back(delegate);
      break;
    }
    if (IsMCPServer(delegate) && delegate->tools_attached()) {
      servers.push_back(delegate);
    }
  }
  auto barrier = base::BarrierCallback<FetchedContentTools>(
      servers.size(),
      base::BindOnce(&AssociatedContentManager::OnMCPServerToolInfosFetched,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  for (size_t i = 0; i < servers.size(); ++i) {
    FetchContentTools(servers[i], i, barrier);
  }
}

void AssociatedContentManager::OnToolInfosFetched(
    const url::Origin& origin,
    GetToolInfosCallback callback,
    std::vector<std::unique_ptr<Tool>> tools) {
  tools.resize(std::min(tools.size(), kMaxToolsPerContent));
  std::vector<mojom::ToolInfoPtr> infos;
  infos.reserve(tools.size());
  // Blocked tools are listed too, so the user can see and undo the choice.
  for (const auto& tool : tools) {
    infos.push_back(
        mojom::ToolInfo::New(std::string(tool->DisplayName()),
                             std::string(tool->DisplayDescription()),
                             GetToolPermission(origin, *tool)));
  }
  std::move(callback).Run(std::move(infos));
}

void AssociatedContentManager::OnMCPServerToolInfosFetched(
    GetToolInfosCallback callback,
    std::vector<FetchedContentTools> fetched) {
  std::ranges::sort(fetched, {}, &FetchedContentTools::index);
  std::vector<mojom::ToolInfoPtr> infos;
  size_t mcp_tool_count = 0;
  for (FetchedContentTools& server : fetched) {
    const bool is_listed = &server == &fetched.back();
    server.tools.resize(std::min(server.tools.size(), kMaxToolsPerContent));
    // Counted as AddToolsForGenerationLoop() counts them, but blocked tools
    // are listed too, so the user can see and undo the choice.
    for (const std::unique_ptr<Tool>& tool : server.tools) {
      const mojom::ToolPermission permission =
          GetToolPermission(server.origin, *tool);
      if (permission != mojom::ToolPermission::kNeverAllow) {
        if (mcp_tool_count >= kMaxMCPToolsPerGenerationLoop) {
          break;
        }
        ++mcp_tool_count;
      }
      if (is_listed) {
        infos.push_back(mojom::ToolInfo::New(
            std::string(tool->DisplayName()),
            std::string(tool->DisplayDescription()), permission));
      }
    }
  }
  std::move(callback).Run(std::move(infos));
}

void AssociatedContentManager::SetToolPermission(
    std::string_view content_uuid,
    std::string_view tool_name,
    std::string_view tool_description,
    mojom::ToolPermission permission) {
  auto it = std::ranges::find_if(content_delegates_,
                                 [&content_uuid](const auto& delegate) {
                                   return delegate->uuid() == content_uuid;
                                 });
  if (it == content_delegates_.end()) {
    return;
  }

  // Each url::Origin::Create() of an opaque origin gets a fresh nonce, so
  // there's no key to record the choice against that could be found again.
  const url::Origin origin = (*it)->GetOrigin();
  if (origin.opaque()) {
    return;
  }

  // kAsk is the default, so drop the entry rather than storing it.
  if (permission == mojom::ToolPermission::kAsk) {
    auto* tool_permissions =
        base::FindOrNull(content_tool_permissions_, origin);
    if (!tool_permissions) {
      return;
    }
    tool_permissions->erase(tool_name);
    if (tool_permissions->empty()) {
      content_tool_permissions_.erase(origin);
    }
  } else {
    content_tool_permissions_[origin][std::string(tool_name)] = {
        .permission = permission, .description = std::string(tool_description)};
  }

  // The choice is conversation-wide, so tell every UI bound to the
  // conversation rather than leaving the one it was made from to update
  // itself.
  PushToolInfos(*it);
}

void AssociatedContentManager::SetToolPermissionForModelToolName(
    std::string_view model_tool_name,
    mojom::ToolPermission permission) {
  auto tool_it = std::ranges::find_if(tools_, [&](const auto& entry) {
    return entry.tool->Name() == model_tool_name;
  });
  if (tool_it == tools_.end() || !tool_it->records_choice) {
    return;
  }

  // Routed through the content which exposed the tool, so the choice is keyed
  // and announced exactly like one made in the dialog. The description is the
  // one the model was given for the tool the user was asked about.
  auto delegate_it =
      std::ranges::find_if(content_delegates_, [&](const auto& delegate) {
        return delegate->GetOrigin() == tool_it->origin;
      });
  if (delegate_it == content_delegates_.end()) {
    return;
  }
  SetToolPermission((*delegate_it)->uuid(), tool_it->tool->DisplayName(),
                    tool_it->tool->DisplayDescription(), permission);
}

void AssociatedContentManager::NotifyContentToolsChanged(
    const std::string& content_uuid,
    std::vector<mojom::ToolInfoPtr> tools) {
  conversation_->OnContentToolsChanged(content_uuid, std::move(tools));
}

mojom::ToolPermission AssociatedContentManager::GetToolPermission(
    const url::Origin& origin,
    const Tool& tool) const {
  auto* tool_permissions = base::FindOrNull(content_tool_permissions_, origin);
  if (!tool_permissions) {
    return mojom::ToolPermission::kAsk;
  }
  const ContentToolPermission* choice =
      base::FindOrNull(*tool_permissions, tool.DisplayName());
  if (!choice) {
    return mojom::ToolPermission::kAsk;
  }
  // The user allowed the tool as it was described to them, so a tool since
  // re-registered under the same name with another description has to be
  // asked about again.
  if (choice->permission == mojom::ToolPermission::kAllowSession &&
      choice->description != tool.DisplayDescription()) {
    return mojom::ToolPermission::kAsk;
  }
  return choice->permission;
}

void AssociatedContentManager::DetectContentTools(
    AssociatedContentDelegate* delegate) {
  delegate->GetContentTools(
      base::BindOnce(&AssociatedContentManager::OnContentToolsDetected,
                     weak_ptr_factory_.GetWeakPtr(), delegate->GetWeakPtr()));
}

void AssociatedContentManager::OnContentToolsDetected(
    base::WeakPtr<AssociatedContentDelegate> delegate,
    std::vector<std::unique_ptr<Tool>> tools) {
  if (!delegate) {
    return;
  }
  bool tools_attached = !tools.empty();
  if (delegate->tools_attached() == tools_attached) {
    return;
  }
  delegate->set_tools_attached(tools_attached);
}

bool AssociatedContentManager::IsEligibleForAutoToolsUpdate(
    const std::string& content_uuid) const {
  return !content_uuid_to_conversation_turns_.contains(content_uuid) &&
         !tools_attachment_overridden_.contains(content_uuid);
}

void AssociatedContentManager::OnContentToolsChanged(
    AssociatedContentDelegate* delegate) {
  // An MCP server's tools stay attached whatever it offers, so what changes is
  // the list the UI shows, and the loop if it went without them.
  if (IsMCPServer(delegate)) {
    PushToolInfos(delegate);
    MaybeFetchToolsForGenerationLoop(delegate);
    return;
  }
  if (!IsEligibleForAutoToolsUpdate(delegate->uuid())) {
    return;
  }
  DetectContentTools(delegate);
}

void AssociatedContentManager::ClearContent() {
  DVLOG(1) << __func__;
  if (!HasPageContent()) {
    return;
  }

  for (AssociatedContentDelegate* delegate : GetPageContentDelegates()) {
    RemoveContent(delegate, /*notify_updated=*/false);
  }

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::AssociateUnsentContentWithTurn(
    const mojom::ConversationTurnPtr& turn) {
  CHECK(turn->uuid.has_value());

  for (const auto& content : content_delegates_) {
    if (content_uuid_to_conversation_turns_.contains(content->uuid()) ||
        IsMCPServer(content)) {
      continue;
    }
    content_uuid_to_conversation_turns_[content->uuid()] = {
        .turn_uuid = turn->uuid.value()};
  }
}

bool AssociatedContentManager::AssociateContentWithTurn(
    std::string_view content_uuid,
    std::string_view turn_uuid) {
  DVLOG(1) << __func__;

  auto it = std::ranges::find(content_delegates_, content_uuid,
                              [](const auto& ptr) { return ptr->uuid(); });
  if (it == content_delegates_.end() || IsMCPServer(*it)) {
    return false;
  }
  const bool inserted =
      content_uuid_to_conversation_turns_
          .try_emplace(std::string(content_uuid),
                       ContentTurn{.turn_uuid = std::string(turn_uuid)})
          .second;
  if (!inserted) {
    return false;
  }

  conversation_->OnAssociatedContentUpdated();
  return true;
}

void AssociatedContentManager::RemoveContentAssociatedWithTurns(
    const base::flat_set<std::string>& turn_uuids) {
  DVLOG(1) << __func__;

  std::vector<std::string> to_remove;
  for (const auto& [content_uuid, content_turn] :
       content_uuid_to_conversation_turns_) {
    if (turn_uuids.contains(content_turn.turn_uuid)) {
      to_remove.push_back(content_uuid);
    }
  }
  if (to_remove.empty()) {
    return;
  }

  for (const auto& content_uuid : to_remove) {
    content_uuid_to_conversation_turns_.erase(content_uuid);
    RemoveContent(content_uuid, /*notify_updated=*/false);
  }
  conversation_->OnAssociatedContentUpdated();
}

std::vector<mojom::AssociatedContentPtr>
AssociatedContentManager::GetAssociatedContent() const {
  DVLOG(1) << __func__;

  // For the <page></page> wrapper around the content.
  constexpr uint32_t kAdditionalCharsPerContent = 15;
  const uint32_t max_associated_content_length =
      ModelService::CalcuateMaxAssociatedContentLengthForModel(
          conversation_->GetCurrentModel());
  uint32_t total_consumed_chars = 0;

  std::vector<mojom::AssociatedContentPtr> result;
  for (AssociatedContentDelegate* delegate : content_delegates_) {
    auto cached_page_content = delegate->cached_page_content();
    mojom::AssociatedContentPtr content = mojom::AssociatedContent::New();
    content->uuid = delegate->uuid();
    content->content_id = delegate->content_id();
    content->url = delegate->url();
    content->title = base::UTF16ToUTF8(delegate->title());
    content->content_type = cached_page_content.content_type;
    content->tools_attached = delegate->tools_attached();

    // An MCP server sends nothing, so takes nothing from the budget.
    if (IsMCPServer(delegate)) {
      content->content_used_percentage = 100;
      result.push_back(std::move(content));
      continue;
    }

    const uint32_t content_length =
        cached_page_content.content.length() + kAdditionalCharsPerContent;
    if (total_consumed_chars + content_length <=
        max_associated_content_length) {
      content->content_used_percentage = 100;
    } else if (total_consumed_chars >= max_associated_content_length) {
      content->content_used_percentage = 0;
    } else {
      // Convert to float to avoid integer division, which truncates towards
      // zero
      // and could lead to inaccurate results before multiplication.
      float pct = static_cast<float>(max_associated_content_length -
                                     total_consumed_chars) /
                  static_cast<float>(content_length) * 100;
      content->content_used_percentage = base::ClampRound(pct);
    }

    auto it = content_uuid_to_conversation_turns_.find(content->uuid);
    if (it != content_uuid_to_conversation_turns_.end()) {
      content->conversation_turn_uuid = it->second.turn_uuid;
    }

    result.push_back(std::move(content));
    total_consumed_chars += content_length;
  }
  return result;
}

void AssociatedContentManager::HasContentUpdated(
    base::OnceCallback<void(bool)> callback) {
  DVLOG(1) << __func__;

  std::vector<PageContent> cached_content;
  std::ranges::copy(GetCachedContents(), std::back_inserter(cached_content));

  GetContent(base::BindOnce(
      [](base::WeakPtr<AssociatedContentManager> self,
         std::vector<PageContent> cached_content,
         base::OnceCallback<void(bool)> callback) {
        if (!self) {
          return;
        }

        auto new_contents = self->GetCachedContents();
        bool changed = cached_content.size() != new_contents.size();
        if (!changed) {
          for (size_t i = 0; i < cached_content.size(); ++i) {
            auto cached = cached_content[i];
            auto& new_content = new_contents[i].get();
            if (cached != new_content) {
              changed = true;
              break;
            }
          }
        }

        std::move(callback).Run(changed);
      },
      weak_ptr_factory_.GetWeakPtr(), std::move(cached_content),
      std::move(callback)));
}

void AssociatedContentManager::GetContent(base::OnceClosure callback) {
  DVLOG(1) << __func__;

  // Note: |GetContent| on the AssociatedContentDelegate's is sometimes sync,
  // sometimes async depending on whether it has already been run. This means we
  // need to make sure we don't destroy the signal before we post this callback.
  if (!on_page_text_fetch_complete_) {
    on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();

    // Note: When we create the signal here its important we post the callback
    // before running the BarrierCallback, which will null out the signal when
    // it completes (if all the |GetContent| calls are synchronous).
    on_page_text_fetch_complete_->Post(FROM_HERE, std::move(callback));

    // wait for all GetPageContent to finish
    auto content_callback = base::BarrierClosure(
        content_delegates_.size(),
        base::BindOnce(
            [](base::WeakPtr<AssociatedContentManager> self) {
              if (!self) {
                return;
              }

              self->on_page_text_fetch_complete_->Signal();
              self->on_page_text_fetch_complete_ = nullptr;
            },
            weak_ptr_factory_.GetWeakPtr()));
    for (AssociatedContentDelegate* content : content_delegates_) {
      content->GetContent(base::BindOnce(
          [](base::RepeatingClosure callback, PageContent) { callback.Run(); },
          content_callback));
    }
  } else {
    on_page_text_fetch_complete_->Post(FROM_HERE, std::move(callback));
  }
}

std::vector<std::string>
AssociatedContentManager::GetContentUuidsRequiringScreenshots() const {
  std::vector<std::string> content_uuids;
  for (AssociatedContentDelegate* delegate : content_delegates_) {
    // Only pages can be shown in screenshots, not e.g. a workspace or video.
    const PageContent& page_content = delegate->cached_page_content();
    if (page_content.content_type == mojom::ContentType::PageContent &&
        base::TrimWhitespaceASCII(page_content.content, base::TRIM_ALL)
            .empty()) {
      content_uuids.push_back(delegate->uuid());
    }
  }
  return content_uuids;
}

void AssociatedContentManager::GetScreenshotsOfContent(
    const std::vector<std::string>& content_uuids,
    GetScreenshotsOfContentCallback callback) {
  DVLOG(1) << __func__;

  std::vector<AssociatedContentDelegate*> delegates;
  for (AssociatedContentDelegate* delegate : content_delegates_) {
    if (std::ranges::contains(content_uuids, delegate->uuid())) {
      delegates.push_back(delegate);
    }
  }
  if (delegates.empty()) {
    std::move(callback).Run({});
    return;
  }

  // Each content's screenshots are tagged with its index and put back at it,
  // so they keep the content order whichever content answers first.
  using IndexedScreenshots =
      std::pair<size_t, std::optional<std::vector<mojom::UploadedFilePtr>>>;
  auto uuids = base::ToVector(delegates, &AssociatedContentDelegate::uuid);
  auto barrier = base::BarrierCallback<IndexedScreenshots>(
      delegates.size(),
      base::BindOnce(
          [](GetScreenshotsOfContentCallback callback,
             std::vector<std::string> uuids,
             std::vector<IndexedScreenshots> results) {
            std::vector<std::optional<std::vector<mojom::UploadedFilePtr>>>
                ordered(uuids.size());
            for (auto& [index, screenshots] : results) {
              ordered[index] = std::move(screenshots);
            }
            ContentScreenshots content_screenshots;
            for (size_t i = 0; i < uuids.size(); ++i) {
              if (ordered[i] && !ordered[i]->empty()) {
                content_screenshots.emplace_back(std::move(uuids[i]),
                                                 std::move(*ordered[i]));
              }
            }
            std::move(callback).Run(std::move(content_screenshots));
          },
          std::move(callback), std::move(uuids)));

  for (size_t i = 0; i < delegates.size(); ++i) {
    delegates[i]->GetScreenshots(base::BindOnce(
        [](base::RepeatingCallback<void(IndexedScreenshots)> barrier,
           size_t index,
           std::optional<std::vector<mojom::UploadedFilePtr>> screenshots) {
          barrier.Run({index, std::move(screenshots)});
        },
        barrier, i));
  }
}

std::optional<std::string> AssociatedContentManager::GetContentTurnUuid(
    std::string_view content_uuid) const {
  auto it = content_uuid_to_conversation_turns_.find(content_uuid);
  if (it == content_uuid_to_conversation_turns_.end()) {
    return std::nullopt;
  }
  return it->second.turn_uuid;
}

bool AssociatedContentManager::HasAutoScreenshots(
    std::string_view content_uuid) const {
  auto it = content_uuid_to_conversation_turns_.find(content_uuid);
  return it != content_uuid_to_conversation_turns_.end() &&
         it->second.has_auto_screenshots;
}

void AssociatedContentManager::SetHasAutoScreenshots(
    std::string_view content_uuid) {
  auto it = content_uuid_to_conversation_turns_.find(content_uuid);
  if (it != content_uuid_to_conversation_turns_.end()) {
    it->second.has_auto_screenshots = true;
  }
}

void AssociatedContentManager::GetStagedEntriesFromContent(
    GetStagedEntriesCallback callback) {
  DVLOG(1) << __func__;

  const std::vector<AssociatedContentDelegate*> page_content =
      GetPageContentDelegates();
  if (page_content.size() != 1) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  page_content[0]->GetStagedEntriesFromContent(std::move(callback));
}

PageContents AssociatedContentManager::GetCachedContents() const {
  DVLOG(1) << __func__;
  PageContents result;
  for (AssociatedContentDelegate* delegate : content_delegates_) {
    result.push_back(delegate->cached_page_content());
  }

  return result;
}

PageContentsMap AssociatedContentManager::GetCachedContentsMap() const {
  PageContentsMap result;

  auto contents = GetCachedContents();
  auto meta = GetAssociatedContent();

  for (size_t i = 0; i < contents.size(); ++i) {
    auto turn_id = meta[i]->conversation_turn_uuid;
    if (!turn_id) {
      continue;
    }
    result[turn_id.value()].push_back(contents[i]);
  }

  return result;
}

bool AssociatedContentManager::HasOpenAIChatPermission() const {
  DVLOG(1) << __func__;

  const std::vector<AssociatedContentDelegate*> page_content =
      GetPageContentDelegates();
  return page_content.size() == 1 && page_content[0]->HasOpenAIChatPermission();
}

bool AssociatedContentManager::HasLiveContent() const {
  DVLOG(1) << __func__;

  return owned_content_.size() < content_delegates_.size();
}

bool AssociatedContentManager::HasAssociatedContent() const {
  DVLOG(1) << __func__;

  return !content_delegates_.empty();
}

bool AssociatedContentManager::HasPageContent() const {
  return std::ranges::any_of(content_delegates_,
                             [this](const AssociatedContentDelegate* delegate) {
                               return !IsMCPServer(delegate);
                             });
}

bool AssociatedContentManager::IsVideo() const {
  DVLOG(1) << __func__;

  const std::vector<AssociatedContentDelegate*> page_content =
      GetPageContentDelegates();
  return page_content.size() == 1 &&
         page_content[0]->cached_page_content().content_type ==
             mojom::ContentType::VideoTranscript;
}

size_t AssociatedContentManager::GetPageContentCount() const {
  return GetPageContentDelegates().size();
}

void AssociatedContentManager::OnDestroyed(
    AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  // Note: creating an archive removes the reference to |delegate| from
  // |content_delegates_| and replaces it with an archive.
  CreateArchiveContent(delegate);
}

void AssociatedContentManager::OnRequestArchive(
    AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  CreateArchiveContent(delegate);
}

void AssociatedContentManager::OnNewPage(AssociatedContentDelegate* delegate) {
  // A tab's content is archived, and so no longer observed, before it tells of
  // a new page. An MCP server tells of one when it moves to another endpoint,
  // where the choices made about its tools don't apply, nor does an answer to
  // a prompt about one of them from before. It tells of its tools there once
  // it has them.
  if (!IsMCPServer(delegate)) {
    return;
  }
  const url::Origin origin = delegate->GetOrigin();
  content_tool_permissions_.erase(origin);
  for (GenerationLoopTool& entry : tools_) {
    if (entry.origin == origin) {
      entry.records_choice = false;
    }
  }
}

void AssociatedContentManager::OnTitleChanged(
    AssociatedContentDelegate* delegate) {
  DVLOG(1) << __func__;

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::OnToolsAttachedChanged(
    AssociatedContentDelegate* delegate) {
  // Content attached after the generation loop started (e.g. added by a tool)
  // has its tools added to that loop too, rather than waiting for the next.
  MaybeFetchToolsForGenerationLoop(delegate);
  // Whether its tools count against the cap across servers changes what the
  // servers after it offer.
  if (IsMCPServer(delegate)) {
    PushMCPServerToolInfos(GetMCPServersFrom(delegate, /*inclusive=*/false));
  }

  conversation_->OnAssociatedContentUpdated();
}

void AssociatedContentManager::UpdateToolsForNewGenerationLoop(
    base::OnceClosure on_updated) {
  tools_.clear();
  loop_content_uuids_.clear();
  ++generation_loop_id_;

  // Only load tools from content the user has attached.
  std::vector<AssociatedContentDelegate*> attached_delegates;
  for (AssociatedContentDelegate* content : content_delegates_) {
    if (content->tools_attached() && CanFetchTools(content)) {
      attached_delegates.push_back(content);
    }
  }

  if (attached_delegates.empty()) {
    std::move(on_updated).Run();
    return;
  }

  // The tools are added once all the content has answered, in the order it was
  // attached, so that which MCP tools the cap drops doesn't depend on which
  // server answers first.
  auto barrier = base::BarrierCallback<FetchedContentTools>(
      attached_delegates.size(),
      base::BindOnce(&AssociatedContentManager::OnGenerationLoopToolsFetched,
                     weak_ptr_factory_.GetWeakPtr(), generation_loop_id_,
                     std::move(on_updated)));
  for (size_t i = 0; i < attached_delegates.size(); ++i) {
    FetchContentTools(attached_delegates[i], i, barrier);
  }
}

void AssociatedContentManager::FetchContentTools(
    AssociatedContentDelegate* content,
    size_t index,
    base::RepeatingCallback<void(FetchedContentTools)> done) {
  FetchedContentTools fetched;
  fetched.index = index;
  fetched.content_uuid = content->uuid();
  fetched.origin = content->GetOrigin();
  fetched.is_mcp_server = IsMCPServer(content);
  content->GetContentTools(base::BindOnce(
      [](base::RepeatingCallback<void(FetchedContentTools)> done,
         FetchedContentTools fetched,
         std::vector<std::unique_ptr<Tool>> tools) {
        fetched.tools = std::move(tools);
        done.Run(std::move(fetched));
      },
      std::move(done), std::move(fetched)));
}

void AssociatedContentManager::OnGenerationLoopToolsFetched(
    uint64_t generation_loop_id,
    base::OnceClosure on_updated,
    std::vector<FetchedContentTools> fetched) {
  // A loop that's been replaced isn't waited for.
  if (generation_loop_id != generation_loop_id_) {
    return;
  }
  std::ranges::sort(fetched, {}, &FetchedContentTools::index);
  for (FetchedContentTools& content : fetched) {
    // Left out if it's been detached since, or its tools have been added by
    // another fetch, as when it's detached and attached again.
    auto delegate_it =
        std::ranges::find(content_delegates_, content.content_uuid,
                          [](const auto& ptr) { return ptr->uuid(); });
    if (delegate_it == content_delegates_.end() ||
        !(*delegate_it)->tools_attached() ||
        loop_content_uuids_.contains(content.content_uuid)) {
      continue;
    }
    AddToolsForGenerationLoop(content.content_uuid, content.origin,
                              content.is_mcp_server, std::move(content.tools));
  }
  std::move(on_updated).Run();
}

void AssociatedContentManager::RunWhenContentToolsAdded(
    std::string_view content_uuid,
    ContentToolsAddedCallback callback) {
  auto it = std::ranges::find_if(content_delegates_,
                                 [&content_uuid](const auto& delegate) {
                                   return delegate->uuid() == content_uuid;
                                 });
  // Attached content's tools are already in the loop (or being fetched for it),
  // so there's nothing to wait for.
  if (it == content_delegates_.end() || (*it)->tools_attached()) {
    const size_t tool_count = it == content_delegates_.end()
                                  ? 0
                                  : GetLoopToolCount((*it)->GetOrigin());
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), tool_count));
    return;
  }

  content_tools_added_callbacks_[std::string(content_uuid)].push_back(
      std::move(callback));
}

void AssociatedContentManager::OnAttachedContentToolsFetched(
    base::WeakPtr<AssociatedContentDelegate> delegate,
    uint64_t generation_loop_id,
    std::vector<std::unique_ptr<Tool>> tools) {
  // Dropped if a loop has started since, as it fetched the content's tools
  // itself, or if its origin's tools have been added already (e.g. the content
  // was detached and reattached while fetching), or the content has gone or
  // been detached.
  if (!delegate || generation_loop_id != generation_loop_id_ ||
      GetLoopToolCount(delegate->GetOrigin()) > 0 ||
      !delegate->tools_attached() ||
      std::ranges::find(content_delegates_, delegate.get()) ==
          content_delegates_.end()) {
    return;
  }
  AddToolsForGenerationLoop(delegate->uuid(), delegate->GetOrigin(),
                            IsMCPServer(delegate.get()), std::move(tools));
}

void AssociatedContentManager::AddToolsForGenerationLoop(
    const std::string& content_uuid,
    const url::Origin& origin,
    bool is_mcp_server,
    std::vector<std::unique_ptr<Tool>> tools) {
  const size_t initial_tool_count = tools_.size();
  size_t mcp_tool_count = static_cast<size_t>(
      std::ranges::count(tools_, true, &GenerationLoopTool::is_mcp_server));
  loop_content_uuids_.insert(content_uuid);

  // Cap before filtering so the list the model sees is a subset of the one the
  // website tools dialog shows, which caps the same way.
  tools.resize(std::min(tools.size(), kMaxToolsPerContent));
  for (auto& tool : tools) {
    const mojom::ToolPermission permission = GetToolPermission(origin, *tool);
    // Withheld rather than failed when called, so the model can't waste a turn
    // asking for something it will never be given.
    if (permission == mojom::ToolPermission::kNeverAllow) {
      continue;
    }
    // The servers attached first keep their tools.
    if (is_mcp_server) {
      if (mcp_tool_count >= kMaxMCPToolsPerGenerationLoop) {
        break;
      }
      ++mcp_tool_count;
    }
    tool->SetUserPermissionStrategy(permission);
    tools_.push_back({std::move(tool), origin, is_mcp_server});
  }

  RunContentToolsAddedCallbacks(content_uuid,
                                tools_.size() - initial_tool_count);
}

size_t AssociatedContentManager::GetLoopToolCount(
    const url::Origin& origin) const {
  return static_cast<size_t>(
      std::ranges::count(tools_, origin, &GenerationLoopTool::origin));
}

void AssociatedContentManager::RunContentToolsAddedCallbacks(
    const std::string& content_uuid,
    size_t tool_count) {
  auto it = content_tools_added_callbacks_.find(content_uuid);
  if (it == content_tools_added_callbacks_.end()) {
    return;
  }
  std::vector<ContentToolsAddedCallback> callbacks = std::move(it->second);
  content_tools_added_callbacks_.erase(it);
  for (auto& callback : callbacks) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), tool_count));
  }
}

std::vector<base::WeakPtr<Tool>> AssociatedContentManager::GetTools() {
  std::vector<base::WeakPtr<Tool>> tool_ptrs;
  tool_ptrs.reserve(tools_.size());
  for (const auto& entry : tools_) {
    tool_ptrs.push_back(entry.tool->GetWeakPtr());
  }
  return tool_ptrs;
}

void AssociatedContentManager::StopAllTasks() {
  for (const auto& entry : tools_) {
    entry.tool->StopToolUse();
  }
}

void AssociatedContentManager::DetachContent() {
  DVLOG(1) << __func__;

  content_observations_.RemoveAllObservations();
  for (const AssociatedContentDelegate* delegate : content_delegates_) {
    RunContentToolsAddedCallbacks(delegate->uuid(), 0);
  }
  content_delegates_.clear();
  owned_content_.clear();
  tools_attachment_overridden_.clear();
  mcp_server_ids_.clear();
  content_tool_permissions_.clear();
}

bool AssociatedContentManager::IsMCPServer(
    const AssociatedContentDelegate* delegate) const {
  return mcp_server_ids_.contains(delegate->uuid());
}

std::vector<AssociatedContentDelegate*>
AssociatedContentManager::GetPageContentDelegates() const {
  std::vector<AssociatedContentDelegate*> page_content;
  for (AssociatedContentDelegate* delegate : content_delegates_) {
    if (!IsMCPServer(delegate)) {
      page_content.push_back(delegate);
    }
  }
  return page_content;
}

bool AssociatedContentManager::CanFetchTools(
    const AssociatedContentDelegate* delegate) const {
  // An MCP server's are fetched from the server, so only for a model that can
  // use them.
  return !IsMCPServer(delegate) ||
         conversation_->GetCurrentModel().supports_tools;
}

void AssociatedContentManager::MaybeFetchToolsForGenerationLoop(
    AssociatedContentDelegate* delegate) {
  if (!delegate->tools_attached() ||
      GetLoopToolCount(delegate->GetOrigin()) > 0 || !CanFetchTools(delegate)) {
    return;
  }
  delegate->GetContentTools(
      base::BindOnce(&AssociatedContentManager::OnAttachedContentToolsFetched,
                     weak_ptr_factory_.GetWeakPtr(), delegate->GetWeakPtr(),
                     generation_loop_id_));
}

std::vector<std::string> AssociatedContentManager::GetMCPServersFrom(
    const AssociatedContentDelegate* delegate,
    bool inclusive) const {
  std::vector<std::string> uuids;
  auto it = std::ranges::find(content_delegates_, delegate,
                              [](const auto& ptr) { return ptr.get(); });
  if (it != content_delegates_.end() && !inclusive) {
    ++it;
  }
  for (; it != content_delegates_.end(); ++it) {
    if (IsMCPServer(*it)) {
      uuids.push_back((*it)->uuid());
    }
  }
  return uuids;
}

void AssociatedContentManager::PushToolInfos(
    const AssociatedContentDelegate* delegate) {
  if (!IsMCPServer(delegate)) {
    GetToolInfos(
        delegate->uuid(),
        base::BindOnce(&AssociatedContentManager::NotifyContentToolsChanged,
                       weak_ptr_factory_.GetWeakPtr(), delegate->uuid()));
    return;
  }
  // The cap across servers cuts the tools of the servers after it, once its own
  // are counted.
  PushMCPServerToolInfos(GetMCPServersFrom(delegate, /*inclusive=*/true));
}

void AssociatedContentManager::PushMCPServerToolInfos(
    const std::vector<std::string>& server_uuids) {
  for (const std::string& uuid : server_uuids) {
    GetToolInfos(uuid, base::BindOnce(
                           &AssociatedContentManager::NotifyContentToolsChanged,
                           weak_ptr_factory_.GetWeakPtr(), uuid));
  }
}

bool AssociatedContentManager::HasLiveContentForOrigin(
    const url::Origin& origin) const {
  for (AssociatedContentDelegate* delegate : content_delegates_) {
    // Archived content can't expose tools.
    auto owned_it =
        std::ranges::find(owned_content_, delegate,
                          [](const auto& owned) { return owned.get(); });
    if (owned_it != owned_content_.end()) {
      continue;
    }
    if (delegate->GetOrigin() == origin) {
      return true;
    }
  }
  return false;
}

void AssociatedContentManager::MaybeResetToolPermissionsForOrigin(
    const url::Origin& origin) {
  auto origin_it = content_tool_permissions_.find(origin);
  if (origin_it == content_tool_permissions_.end() ||
      HasLiveContentForOrigin(origin)) {
    return;
  }
  content_tool_permissions_.erase(origin_it);
}

}  // namespace ai_chat
