/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_autoplay_blocked_image_model.h"

#include <memory>

#include "brave/browser/ui/content_settings/brave_autoplay_content_setting_bubble_model.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/web_contents.h"

using content::WebContents;

BraveAutoplayBlockedImageModel::BraveAutoplayBlockedImageModel()
    : ContentSettingSimpleImageModel(ImageType::MEDIASTREAM,
                                     ContentSettingsType::AUTOPLAY) {}

bool BraveAutoplayBlockedImageModel::UpdateAndGetVisibility(
    WebContents* web_contents) {
  if (!web_contents)
    return false;

  content_settings::PageSpecificContentSettings* content_settings =
      content_settings::PageSpecificContentSettings::GetForFrame(
          web_contents->GetPrimaryMainFrame());
  if (!content_settings)
    return false;
  if (!content_settings->IsContentBlocked(content_type()))
    return false;

  SetIcon(ContentSettingsType::AUTOPLAY, /*blocked=*/true);
  set_explanatory_string_id(IDS_BLOCKED_AUTOPLAY_TITLE);
  set_tooltip(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_BLOCKED_AUTOPLAY_TITLE));
  return true;
}

std::unique_ptr<ContentSettingBubbleModel>
BraveAutoplayBlockedImageModel::CreateBubbleModelImpl(
    ContentSettingBubbleModel::Delegate* delegate,
    content::WebContents* web_contents) {
  return std::make_unique<BraveAutoplayContentSettingBubbleModel>(delegate,
                                                                  web_contents);
}
