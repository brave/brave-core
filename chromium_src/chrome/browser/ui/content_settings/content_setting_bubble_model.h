/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_MODEL_H_

#define CreateContentSettingBubbleModel                       \
  CreateContentSettingBubbleModel_ChromiumImpl(               \
      Delegate* delegate, content::WebContents* web_contents, \
      ContentSettingsType content_type);                      \
  static std::unique_ptr<ContentSettingBubbleModel>           \
      CreateContentSettingBubbleModel

#include "../../../../../../chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#undef CreateContentSettingBubbleModel

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_MODEL_H_
