// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_BUBBLE_HOST_TAB_H_
#define BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_BUBBLE_HOST_TAB_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

// The requesting tab a wallet panel bubble serves, stamped on the bubble's
// WebContents at creation so the panel WebUI resolves its served tab per use.
class WalletBubbleHostTab
    : public content::WebContentsUserData<WalletBubbleHostTab> {
 public:
  WalletBubbleHostTab(const WalletBubbleHostTab&) = delete;
  WalletBubbleHostTab& operator=(const WalletBubbleHostTab&) = delete;
  ~WalletBubbleHostTab() override;

  content::WebContents* tab() { return tab_.get(); }

 private:
  friend class content::WebContentsUserData<WalletBubbleHostTab>;
  explicit WalletBubbleHostTab(content::WebContents* host,
                               base::WeakPtr<content::WebContents> tab);

  base::WeakPtr<content::WebContents> tab_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_BRAVE_WALLET_WALLET_BUBBLE_HOST_TAB_H_
