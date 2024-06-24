/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_BUBBLE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_BUBBLE_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class BraveNewsBubbleView;
class BraveNewsActionIconView;

namespace brave_news {

class BraveNewsBubbleController
    : public content::WebContentsUserData<BraveNewsBubbleController> {
 public:
  static BraveNewsBubbleController* CreateOrGetFromWebContents(
      content::WebContents* web_contents);

  ~BraveNewsBubbleController() override;

  void ShowBubble(base::WeakPtr<BraveNewsActionIconView> anchor_view);
  BraveNewsBubbleView* GetBubble();
  void OnBubbleClosed();
  base::WeakPtr<BraveNewsBubbleController> AsWeakPtr();

 private:
  friend class content::WebContentsUserData<BraveNewsBubbleController>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  explicit BraveNewsBubbleController(content::WebContents* web_contents);

  raw_ptr<BraveNewsBubbleView> bubble_ = nullptr;
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  base::WeakPtrFactory<BraveNewsBubbleController> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_BUBBLE_CONTROLLER_H_
