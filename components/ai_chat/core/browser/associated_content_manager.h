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

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

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
class AssociatedContentManager : public AssociatedContentDelegate::Observer {
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

  // AssociatedContentDelegate::Observer:
  void OnRequestArchive(AssociatedContentDelegate* delegate) override;
  void OnDestroyed(AssociatedContentDelegate* delegate) override;
  void OnTitleChanged(AssociatedContentDelegate* delegate) override;

  std::vector<AssociatedContentDelegate*> GetContentDelegatesForTesting() {
    return content_delegates_;
  }

 private:
  void DetachContent();

  raw_ptr<ConversationHandler> conversation_;

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
