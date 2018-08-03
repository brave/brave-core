/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_AUTOPLAY_CONTENT_SETTING_BUBBLE_MODEL_H_
#define BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_AUTOPLAY_CONTENT_SETTING_BUBBLE_MODEL_H_

#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"

class Profile;

using content::WebContents;

class BraveAutoplayContentSettingBubbleModel : public ContentSettingSimpleBubbleModel {
 public:
   BraveAutoplayContentSettingBubbleModel(Delegate* delegate,
                                          WebContents* web_contents,
                                          Profile* profile);
   ~BraveAutoplayContentSettingBubbleModel() override;

 protected:
  bool settings_changed() const;
  int selected_item() const { return selected_item_; }

 private:
  void SetTitle();
  void SetRadioGroup();
  void SetNarrowestContentSetting(ContentSetting setting);
  void OnRadioClicked(int radio_index) override;

  ContentSetting block_setting_;
  int selected_item_;

  DISALLOW_COPY_AND_ASSIGN(BraveAutoplayContentSettingBubbleModel);
};

#endif // BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_AUTOPLAY_CONTENT_SETTING_BUBBLE_MODEL_H_
