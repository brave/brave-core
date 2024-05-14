/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_BUBBLE_HOST_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_BUBBLE_HOST_H_

#include <memory>

#include "base/callback_list.h"
#include "chrome/browser/ui/browser_user_data.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"

namespace content {
class WebContents;
}

namespace brave_shields {

// A browser helper responsible for displaying a bubble inviting the user to
// enable the "CookieList" AdBlock filter, which will block obtrusive cookie
// consent notices. Instances own the displayed bubble, and are owned by a
// Browser.
class CookieListOptInBubbleHost
    : public BrowserUserData<CookieListOptInBubbleHost>,
      public TabStripModelObserver {
 public:
  explicit CookieListOptInBubbleHost(Browser* browser);

  CookieListOptInBubbleHost(const CookieListOptInBubbleHost&) = delete;
  CookieListOptInBubbleHost& operator=(const CookieListOptInBubbleHost&) =
      delete;

  ~CookieListOptInBubbleHost() override;

  static void MaybeCreateForBrowser(Browser* browser);

  void ShowBubble();

  content::WebContents* GetBubbleWebContentsForTesting();

  static void AllowBubbleInBackgroundForTesting();

 private:
  friend class BrowserUserData<CookieListOptInBubbleHost>;

  // TabStripObserver:
  void TabChangedAt(content::WebContents* web_contents,
                    int index,
                    TabChangeType change_type) override;

  std::unique_ptr<WebUIBubbleManager> bubble_manager_;
  base::CallbackListSubscription session_restored_subscription_;

  BROWSER_USER_DATA_KEY_DECL();
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_BUBBLE_HOST_H_
