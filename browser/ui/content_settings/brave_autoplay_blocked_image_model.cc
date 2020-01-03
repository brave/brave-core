/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_autoplay_blocked_image_model.h"

#include "brave/browser/ui/content_settings/brave_autoplay_content_setting_bubble_model.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"

using content::WebContents;

BraveAutoplayBlockedImageModel::BraveAutoplayBlockedImageModel()
    : ContentSettingSimpleImageModel(ImageType::PLUGINS,
                                     ContentSettingsType::AUTOPLAY) {}

bool BraveAutoplayBlockedImageModel::UpdateAndGetVisibility(WebContents* web_contents) {
  if (!web_contents)
    return false;

  TabSpecificContentSettings* content_settings =
      TabSpecificContentSettings::FromWebContents(web_contents);
  if (!content_settings)
    return false;
  if (!content_settings->IsContentBlocked(content_type()))
    return false;

  const gfx::VectorIcon* badge_id = &kBlockedBadgeIcon;
  const gfx::VectorIcon* icon = &kAutoplayStatusIcon;
  set_icon(*icon, *badge_id);
  set_explanatory_string_id(IDS_BLOCKED_AUTOPLAY_TITLE);
  set_tooltip(l10n_util::GetStringUTF16(IDS_BLOCKED_AUTOPLAY_TITLE));
  return true;
}

std::unique_ptr<ContentSettingBubbleModel>
BraveAutoplayBlockedImageModel::CreateBubbleModelImpl(ContentSettingBubbleModel::Delegate* delegate,
      content::WebContents* web_contents) {
  return std::make_unique<BraveAutoplayContentSettingBubbleModel>(delegate, web_contents);
}
