/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"

class BraveContentSettingPluginBubbleModel : public ContentSettingSimpleBubbleModel {
 public:
  BraveContentSettingPluginBubbleModel(Delegate* delegate,
      content::WebContents* web_contents);
 private:
  void OnLearnMoreClicked() override;
  void OnCustomLinkClicked() override;

  void RunPluginsOnPage();

  DISALLOW_COPY_AND_ASSIGN(BraveContentSettingPluginBubbleModel);
};
