/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/content_settings/content_setting_image_model.h"

#include "brave/browser/ui/content_settings/brave_content_setting_image_models.h"
#include "brave/components/vector_icons/vector_icons.h"

#define GenerateContentSettingImageModels \
  GenerateContentSettingImageModels_ChromiumImpl
#include "src/chrome/browser/ui/content_settings/content_setting_image_model.cc"
#undef GenerateContentSettingImageModels

std::vector<std::unique_ptr<ContentSettingImageModel>>
ContentSettingImageModel::GenerateContentSettingImageModels() {
  std::vector<std::unique_ptr<ContentSettingImageModel>> result =
      GenerateContentSettingImageModels_ChromiumImpl();
  BraveGenerateContentSettingImageModels(&result);
  return result;
}

void ContentSettingImageModel::GetIconFromType(
    ContentSettingsType type,
    bool blocked,
    raw_ptr<const gfx::VectorIcon>* icon,
    raw_ptr<const gfx::VectorIcon>* badge) {
  if (type == ContentSettingsType::AUTOPLAY) {
    *badge = (blocked ? &vector_icons::kBlockedBadgeIcon : &gfx::kNoneIcon);
    *icon = &kAutoplayStatusIcon;
  } else {
    ::GetIconFromType(type, blocked, icon, badge);
  }
}
