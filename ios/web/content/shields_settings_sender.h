// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_CONTENT_SHIELDS_SETTINGS_SENDER_H_
#define BRAVE_IOS_WEB_CONTENT_SHIELDS_SETTINGS_SENDER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace brave_shields {

// Pushes Shields settings to each frame's ShieldsSettingsAgent before commit,
// like desktop's BraveShieldsWebContentsObserver.
class ShieldsSettingsSender
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ShieldsSettingsSender> {
 public:
  ShieldsSettingsSender(const ShieldsSettingsSender&) = delete;
  ShieldsSettingsSender& operator=(const ShieldsSettingsSender&) = delete;
  ~ShieldsSettingsSender() override;

  // content::WebContentsObserver:
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  friend WebContentsUserData;
  explicit ShieldsSettingsSender(content::WebContents* web_contents);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_WEB_CONTENT_SHIELDS_SETTINGS_SENDER_H_
