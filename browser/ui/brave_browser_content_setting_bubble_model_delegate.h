/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_CONTENT_SETTING_BUBBLE_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_CONTENT_SETTING_BUBBLE_MODEL_DELEGATE_H_

#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"

class BraveBrowserContentSettingBubbleModelDelegate
    : public BrowserContentSettingBubbleModelDelegate {
 public:
  explicit BraveBrowserContentSettingBubbleModelDelegate(Browser* browser);
  BraveBrowserContentSettingBubbleModelDelegate(
      const BraveBrowserContentSettingBubbleModelDelegate&) = delete;
  BraveBrowserContentSettingBubbleModelDelegate& operator=(
      const BraveBrowserContentSettingBubbleModelDelegate&) = delete;
  ~BraveBrowserContentSettingBubbleModelDelegate() override;

  void ShowWidevineLearnMorePage();
  void ShowLearnMorePage(ContentSettingsType type) override;

 private:
  Browser* const browser_;
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_CONTENT_SETTING_BUBBLE_MODEL_DELEGATE_H_
