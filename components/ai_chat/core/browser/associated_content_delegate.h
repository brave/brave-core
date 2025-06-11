// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_DELEGATE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_DELEGATE_H_

#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

struct PageContent {
  // Note: |content| is not sanitized for use in the backend. Run it through
  // |EngineConsumer::SanitizeInput| before sending it.
  std::string content = "";
  bool is_video = false;
  std::string invalidation_token = "";

  PageContent();
  PageContent(std::string content,
              bool is_video,
              std::string invalidation_token = "");

  PageContent(const PageContent&);
  PageContent(PageContent&&);
  PageContent& operator=(const PageContent&);
  PageContent& operator=(PageContent&&);

  bool operator==(const PageContent& other) const {
    return content == other.content && is_video == other.is_video &&
           invalidation_token == other.invalidation_token;
  }
};

// |invalidation_token| is an optional parameter that will be passed back on
// the next call to |GetPageContent| so that the implementer may determine if
// the page content is static or if it needs to be fetched again. Most page
// content should be fetched again, but some pages are known to be static
// during their lifetime and may have expensive content fetching, e.g. videos
// with transcripts fetched over the network.
using GetPageContentCallback = base::OnceCallback<void(PageContent)>;

// TODO(petemill): consider making SearchQuerySummary generic (StagedEntries)
// or a list of ConversationTurn objects.
using GetStagedEntriesCallback = base::OnceCallback<void(
    const std::optional<std::vector<SearchQuerySummary>>& entries)>;

// TODO(https://github.com/brave/brave-browser/issues/45732): Move this to its
// own file and merge with AssociatedContentDriver.
// Supplements a conversation with associated page content
class AssociatedContentDelegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}
    virtual void OnNavigated(AssociatedContentDelegate* delegate) {}
    virtual void OnTitleChanged(AssociatedContentDelegate* delegate) {}
  };

  AssociatedContentDelegate();
  virtual ~AssociatedContentDelegate();

  // Unique ID for the content. For browser Tab content, this should be
  // a navigation ID that's re-used during back navigations.
  virtual int GetContentId() const = 0;
  // Get metadata about the current page
  virtual GURL GetURL() const = 0;
  virtual std::u16string GetTitle() const = 0;

  // Implementer should fetch content from the "page" associated with this
  // conversation.
  // |is_video| lets the conversation know that the content is focused on
  // video content so that various UI language can be adapted.
  virtual void GetContent(GetPageContentCallback callback) = 0;
  // Get current cache of content, if available. Do not perform any fresh
  // fetch for the content.
  virtual const PageContent& GetCachedPageContent() const = 0;

  // Get summarizer-key meta tag content from Brave Search SERP if exists and
  // use it to fetch search query and summary from Brave search chatllm
  // endpoint.
  virtual void GetStagedEntriesFromContent(GetStagedEntriesCallback callback);
  // Signifies whether the content has permission to open a conversation's UI
  // within the browser.
  virtual bool HasOpenAIChatPermission() const;
  virtual void GetScreenshots(
      mojom::ConversationHandler::GetScreenshotsCallback callback);

  base::WeakPtr<AssociatedContentDelegate> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  virtual void OnTitleChanged();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  const std::string& uuid() const { return uuid_; }
  void set_uuid(std::string uuid) { uuid_ = uuid; }

 protected:
  // Content has navigated
  virtual void OnNewPage(int64_t navigation_id);

 private:
  std::string uuid_;
  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<AssociatedContentDelegate> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_DELEGATE_H_
