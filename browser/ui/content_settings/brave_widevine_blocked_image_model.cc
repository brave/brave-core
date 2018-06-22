/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_widevine_blocked_image_model.h"

#include "brave/common/pref_names.h"
#include "brave/common/shield_exceptions.h"
#include "brave/browser/ui/content_settings/brave_widevine_content_setting_bubble_model.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"

using content::WebContents;

BraveWidevineBlockedImageModel::BraveWidevineBlockedImageModel(
    ImageType image_type,
    ContentSettingsType content_type)
    : ContentSettingSimpleImageModel(image_type, content_type) {}

void BraveWidevineBlockedImageModel::UpdateFromWebContents(
    WebContents* web_contents) {
  set_visible(false);
  if (!web_contents)
    return;

  // If the user alraedy opted in, don't show more UI
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  if (prefs->GetBoolean(kWidevineOptedIn)) {
    return;
  }

  // If the URL isn't one that we whitelist as a site that gets UI for
  // Widevine to be installable, then don't show naything.
  GURL url = web_contents->GetURL();
  if (!brave::IsWidevineInstallableURL(url)) {
    return;
  }

  set_visible(true);
  const gfx::VectorIcon* badge_id = &kBlockedBadgeIcon;
  const gfx::VectorIcon* icon = &kExtensionIcon;
  set_icon(*icon, *badge_id);
  set_explanatory_string_id(IDS_WIDEVINE_NOT_INSTALLED_MESSAGE);
  set_tooltip(l10n_util::GetStringUTF16(IDS_WIDEVINE_NOT_INSTALLED_EXPLANATORY_TEXT));
}

ContentSettingBubbleModel*
BraveWidevineBlockedImageModel::CreateBubbleModelImpl(
    ContentSettingBubbleModel::Delegate* delegate,
    WebContents* web_contents,
    Profile* profile) {
  return new BraveWidevineContentSettingPluginBubbleModel(delegate,
      web_contents, profile);
}
