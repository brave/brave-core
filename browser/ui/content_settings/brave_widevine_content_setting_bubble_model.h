/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"

class Profile;
class BraveBrowserContentSettingBubbleModelDelegate;

class BraveWidevineContentSettingPluginBubbleModel : public ContentSettingSimpleBubbleModel {
 public:
  BraveWidevineContentSettingPluginBubbleModel(ContentSettingBubbleModel::Delegate* delegate,
                                               content::WebContents* web_contents,
                                               Profile* profile);
  BraveBrowserContentSettingBubbleModelDelegate* brave_content_settings_delegate() const {
    return brave_content_settings_delegate_;
  }
  void RunPluginsOnPage();

 private:
  void OnLearnMoreClicked() override;
  void OnCustomLinkClicked() override;
  void SetTitle();
  void SetMessage();
  void SetCustomLink();
  void SetLearnMore();
  void SetManageText();

  BraveBrowserContentSettingBubbleModelDelegate* brave_content_settings_delegate_;

  DISALLOW_COPY_AND_ASSIGN(BraveWidevineContentSettingPluginBubbleModel);
};
