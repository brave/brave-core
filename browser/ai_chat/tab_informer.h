// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TAB_INFORMER_H_
#define BRAVE_BROWSER_AI_CHAT_TAB_INFORMER_H_

#include "content/public/browser/web_contents_observer.h"

namespace content {
class NavigationEntry;
}

namespace ai_chat {

class TabInformerService;

// This class informs the TabInformerService about changes to tabs (i.e.
// creation, deletion, title/url updates). Each instance of this class is
// associated with a single tab.
class TabInformer : public content::WebContentsObserver {
 public:
  explicit TabInformer(content::WebContents* contents);
  ~TabInformer() override;

  TabInformer(const TabInformer&) = delete;
  TabInformer& operator=(const TabInformer&) = delete;

  void PrimaryPageChanged(content::Page& page) override;
  void TitleWasSet(content::NavigationEntry* entry) override;

 private:
  int32_t tab_id_ = 0;
  // Note: This can be null in tests.
  raw_ptr<TabInformerService> service_;

  void UpdateTab();
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TAB_INFORMER_H_
