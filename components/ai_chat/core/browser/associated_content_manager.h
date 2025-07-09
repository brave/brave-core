// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/associated_content_snapshot.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

class ConversationHandler;

using PageContents = std::vector<std::reference_wrapper<const PageContent>>;
using GetAllContentCallback = base::OnceCallback<void(PageContents)>;

// This class is responsible for managing the content associated with a
// conversation. This includes:
// - Adding/removing content
// - Loading snapshotted content
// - Snapshotting content as the user navigates between pages
// - Managing whether content should be used as part of the context
class AssociatedContentManager : public AssociatedContentDelegate::Observer {
 public:
  explicit AssociatedContentManager(ConversationHandler* conversation);
  ~AssociatedContentManager() override;

  // Sets the content from the conversation archive.
  void LoadContentSnapshots(
      const mojom::Conversation* metadata,
      const mojom::ConversationArchivePtr& conversation_archive);

  // Replaces |content_uuid| with a snapshot of the content.
  void SetContentSnapshot(std::string content_uuid,
                          std::string text_content,
                          bool is_video);

  // Adds a content delegate to the list of content delegates.
  // If |notify_updated| is true, the conversation will be notified that the
  // content has been updated.
  // If |detach_existing_content| is true, the current content will be detached
  // and the new content will be set as the only content for this conversation.
  void AddContent(AssociatedContentDelegate* delegate,
                  bool notify_updated = true,
                  bool detach_existing_content = false);

  // Removes a content delegate from the list of content delegates.
  void RemoveContent(AssociatedContentDelegate* delegate,
                     bool notify_updated = true);

  // Removes the content delegate with |content_uuid| from the list of content
  // delegates.
  void RemoveContent(std::string_view content_uuid, bool notify_updated = true);

  // Clears all content from the conversation.
  void ClearContent();

  // Checks if the content has changed from what is stored in the cache.
  void HasContentUpdated(base::OnceCallback<void(bool)> callback);

  // Gets the content for this conversation.
  void GetContent(base::OnceClosure callback);
  void GetScreenshots(
      mojom::ConversationHandler::GetScreenshotsCallback callback);
  void GetStagedEntriesFromContent(GetStagedEntriesCallback callback);

  std::vector<mojom::AssociatedContentPtr> GetAssociatedContent() const;

  PageContents GetCachedContents() const;

  bool HasOpenAIChatPermission() const;
  bool HasLiveContent() const;
  bool HasAssociatedContent() const;

  // Determines if the content for this conversation is a single video.
  // Deprecated: Instead use the |type| field on the associated content.
  // TODO(fallaciousreasoning): Remove this method.
  bool IsVideo() const;

  // AssociatedContentDelegate::Observer:
  void OnNavigated(AssociatedContentDelegate* delegate) override;
  void OnTitleChanged(AssociatedContentDelegate* delegate) override;

  std::vector<AssociatedContentDelegate*> GetContentDelegatesForTesting() {
    return content_delegates_;
  }

 private:
  void DetachContent();

  raw_ptr<ConversationHandler> conversation_;

  std::vector<AssociatedContentDelegate*> content_delegates_;

  // Used for ownership - still stored in the above array.
  std::vector<std::unique_ptr<AssociatedContentSnapshot>> content_snapshots_;

  base::ScopedMultiSourceObservation<AssociatedContentDelegate,
                                     AssociatedContentDelegate::Observer>
      content_observations_{this};

  std::unique_ptr<base::OneShotEvent> on_page_text_fetch_complete_ = nullptr;

  base::WeakPtrFactory<AssociatedContentManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat
#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_
