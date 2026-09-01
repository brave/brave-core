// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/origin.h"

namespace ai_chat {

class ConversationHandler;

using PageContents = std::vector<std::reference_wrapper<const PageContent>>;
using PageContentsMap = base::flat_map<std::string, PageContents>;

// This class is responsible for managing the content associated with a
// conversation. This includes:
// - Adding/removing content
// - Loading archived content
// - Archiving content as the user navigates aways
// - Managing whether content should be used as part of the context
class AssociatedContentManager : public ToolProvider,
                                 public AssociatedContentDelegate::Observer {
 public:
  explicit AssociatedContentManager(ConversationHandler* conversation);
  ~AssociatedContentManager() override;

  // Sets the content from the conversation archive.
  void LoadArchivedContent(
      const mojom::Conversation* metadata,
      const mojom::ConversationArchivePtr& conversation_archive);

  // Replaces |content_uuid| with some archived content.
  void CreateArchiveContent(AssociatedContentDelegate* delegate);

  // Adds a content delegate to the list of content delegates.
  // If |notify_updated| is true, the conversation will be notified that the
  // content has been updated. You might want to avoid notifying if you're
  // adding multiple contents at once, or if you're replacing (i.e. via a
  // RemoveContent followed by an AddContent) and you don't want to tell the
  // ConversationHandler/Frontend about the intermediate states.
  // Additionally |OnAssociatedContentUpdated| will update the conversation
  // metadata, which can be problematic if you aren't expecting it to change.
  // If |detach_existing_content| is true, the current content will be detached
  // and the new content will be set as the only content for this conversation.
  void AddContent(AssociatedContentDelegate* delegate,
                  bool notify_updated = true,
                  bool detach_existing_content = false);
  void AddOwnedContent(std::unique_ptr<AssociatedContentDelegate> delegate,
                       bool notify_updated = true);

  // Removes a content delegate from the list of content delegates.
  void RemoveContent(AssociatedContentDelegate* delegate,
                     bool notify_updated = true);

  // Removes the content delegate with |content_uuid| from the list of content
  // delegates.
  void RemoveContent(std::string_view content_uuid, bool notify_updated = true);

  // Sets whether the tools exposed by the content with |content_uuid| are
  // attached (available to the LLM). Reflects an explicit user choice, e.g.
  // detaching via the tools pill.
  void SetToolsAttached(std::string_view content_uuid, bool tools_attached);

  // Fetches the tools the content with |content_uuid| exposes, described for
  // display to the user. Empty if there's no such content. Capped at the same
  // limit as the tools handed to the LLM, so the UI doesn't overpromise.
  using GetToolInfosCallback =
      base::OnceCallback<void(std::vector<mojom::ToolInfoPtr>)>;
  void GetToolInfos(std::string_view content_uuid,
                    GetToolInfosCallback callback);

  // Records how the tool named |tool_name| exposed by the content with
  // |content_uuid| should be handled. Keyed by the content's origin rather
  // than its uuid, so navigating away drops the choice rather than applying
  // it to whatever loads next.
  void SetToolPermission(std::string_view content_uuid,
                         std::string_view tool_name,
                         mojom::ToolPermission permission);

  // Clears all content from the conversation.
  void ClearContent();

  // Associates all content which hasn't been associated with a turn with
  // |turn|. Note: |turn| must have a non-empty |uuid| field.
  void AssociateUnsentContentWithTurn(const mojom::ConversationTurnPtr& turn);

  // Checks if the content has changed from what is stored in the cache.
  void HasContentUpdated(base::OnceCallback<void(bool)> callback);

  // Gets the content for this conversation.
  void GetContent(base::OnceClosure callback);

  void GetScreenshots(
      mojom::ConversationHandler::GetScreenshotsCallback callback);
  void GetStagedEntriesFromContent(GetStagedEntriesCallback callback);

  std::vector<mojom::AssociatedContentPtr> GetAssociatedContent() const;

  PageContents GetCachedContents() const;

  // Gets a map of |turn_id| to a list of content associated with that turn.
  // Note: Before calling this method, all content should be associated with a
  // turn (i.e. via AssociateUnsentContentWithTurn) or you risk missing content
  // in the map.
  PageContentsMap GetCachedContentsMap() const;

  bool HasOpenAIChatPermission() const;
  bool HasLiveContent() const;
  bool HasAssociatedContent() const;

  // Determines if the content for this conversation is a single video.
  // Deprecated: Instead use the |type| field on the associated content.
  // TODO(fallaciousreasoning): Remove this method.
  bool IsVideo() const;

  // The number of content delegates.
  size_t GetContentDelegateCount() const;

  // ToolProvider:
  void UpdateToolsForNewGenerationLoop(base::OnceClosure on_updated) override;
  std::vector<base::WeakPtr<Tool>> GetTools() override;

  // AssociatedContentDelegate::Observer:
  void OnRequestArchive(AssociatedContentDelegate* delegate) override;
  void OnDestroyed(AssociatedContentDelegate* delegate) override;
  void OnTitleChanged(AssociatedContentDelegate* delegate) override;
  void OnToolsAttachedChanged(AssociatedContentDelegate* delegate) override;

  std::vector<AssociatedContentDelegate*> GetContentDelegatesForTesting() {
    return content_delegates_;
  }

 private:
  void DetachContent();

  // Attaches |delegate| when the tools it exposes are non-empty (and detaches
  // it otherwise), so its tools are surfaced (via the tools pill) before any
  // generation occurs. Invoked with the result of GetContentTools().
  void OnContentToolsDetected(base::WeakPtr<AssociatedContentDelegate> delegate,
                              std::vector<std::unique_ptr<Tool>> tools);

  // Invoked with the result of GetContentTools().
  void OnToolInfosFetched(const url::Origin& origin,
                          GetToolInfosCallback callback,
                          std::vector<std::unique_ptr<Tool>> tools);

  // Invoked with the result of GetToolInfos(), to push the list every UI bound
  // to this conversation should now be showing.
  void NotifyContentToolsChanged(const std::string& content_uuid,
                                 std::vector<mojom::ToolInfoPtr> tools);

  mojom::ToolPermission GetToolPermission(const url::Origin& origin,
                                          std::string_view tool_name) const;

  // Takes ownership of the tools |origin| exposes for the loop that's
  // starting, dropping the ones the user has blocked.
  void AddToolsForGenerationLoop(const url::Origin& origin,
                                 std::vector<std::unique_ptr<Tool>> tools);

  raw_ptr<ConversationHandler> conversation_;

  std::vector<std::unique_ptr<Tool>> tools_;

  // Origin -> tool name -> choice, for anything moved off the kAsk default.
  // Deliberately in-memory and per-conversation: granting a site's tool is a
  // decision about this conversation's context, so it shouldn't silently
  // carry over into the next one.
  base::flat_map<url::Origin,
                 base::flat_map<std::string, mojom::ToolPermission>>
      tool_permissions_;

  std::vector<AssociatedContentDelegate*> content_delegates_;
  base::flat_map<std::string, std::string> content_uuid_to_conversation_turns_;

  // Used for ownership - still stored in the above array.
  // This includes:
  // - Archived content
  // - Link content
  std::vector<std::unique_ptr<AssociatedContentDelegate>> owned_content_;

  base::ScopedMultiSourceObservation<AssociatedContentDelegate,
                                     AssociatedContentDelegate::Observer>
      content_observations_{this};

  std::unique_ptr<base::OneShotEvent> on_page_text_fetch_complete_ = nullptr;

  base::WeakPtrFactory<AssociatedContentManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat
#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_
