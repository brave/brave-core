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
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

struct PageContent {
  // Note: |content| is not sanitized for use in the backend. Run it through
  // |EngineConsumer::SanitizeInput| before sending it.
  std::string content = "";
  bool is_video = false;

  PageContent();
  PageContent(std::string content, bool is_video);

  PageContent(const PageContent&);
  PageContent(PageContent&&);
  PageContent& operator=(const PageContent&);
  PageContent& operator=(PageContent&&);

  bool operator==(const PageContent& other) const {
    return content == other.content && is_video == other.is_video;
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
    // Note: This is called from the destructor of the AssociatedContentDelegate
    // so it is not safe to call any virtual methods.
    virtual void OnDestroyed(AssociatedContentDelegate* delegate) {}
    virtual void OnRequestArchive(AssociatedContentDelegate* delegate) {}
    virtual void OnTitleChanged(AssociatedContentDelegate* delegate) {}
  };

  AssociatedContentDelegate();
  virtual ~AssociatedContentDelegate();

  // Implementer should fetch content from the "page" associated with this
  // conversation.
  // |is_video| lets the conversation know that the content is focused on
  // video content so that various UI language can be adapted.
  virtual void GetContent(GetPageContentCallback callback) = 0;

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

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Unique ID for the content. For browser Tab content, this should be
  // a navigation ID that's re-used during back navigations.
  int content_id() const { return content_id_; }
  const std::string& uuid() const { return uuid_; }

  const std::u16string& title() const { return title_; }
  const GURL& url() const { return url_; }

  // Get current cache of content, if available. Do not perform any fresh
  // fetch for the content.
  const PageContent& cached_page_content() const {
    return cached_page_content_;
  }

 protected:
  // Content has navigated
  virtual void OnNewPage(int64_t navigation_id);

  void set_uuid(std::string uuid) { uuid_ = std::move(uuid); }
  void set_url(GURL url) { url_ = std::move(url); }
  void SetTitle(std::u16string title);
  void set_cached_page_content(PageContent page_content) {
    cached_page_content_ = std::move(page_content);
  }

 private:
  friend class MockAssociatedContent;
  friend class MockAssociatedContentDriver;

  int content_id_ = -1;

  std::string uuid_;
  base::ObserverList<Observer> observers_;

  std::u16string title_;
  GURL url_;
  PageContent cached_page_content_;

  base::WeakPtrFactory<AssociatedContentDelegate> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_DELEGATE_H_
