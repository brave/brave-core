/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/content_settings/content_setting_image_model.h"

#include <algorithm>

#include "base/notreached.h"
#include "brave/browser/ui/content_settings/brave_content_setting_image_models.h"
#include "brave/components/vector_icons/vector_icons.h"

#define GenerateContentSettingImageModels \
  GenerateContentSettingImageModels_ChromiumImpl
#define GetContentSettingImageModelIndexForTesting \
  GetContentSettingImageModelIndexForTesting_ChromiumImpl
#define GetAllElementIdentifiers GetAllElementIdentifiers_ChromiumImpl
#include <chrome/browser/ui/content_settings/content_setting_image_model.cc>
#undef GetAllElementIdentifiers
#undef GetContentSettingImageModelIndexForTesting
#undef GenerateContentSettingImageModels

std::vector<std::unique_ptr<ContentSettingImageModel>>
ContentSettingImageModel::GenerateContentSettingImageModels() {
  std::vector<std::unique_ptr<ContentSettingImageModel>> result =
      GenerateContentSettingImageModels_ChromiumImpl();
  BraveGenerateContentSettingImageModels(&result);
  return result;
}

// static
std::vector<ui::ElementIdentifier>
ContentSettingImageModel::GetAllElementIdentifiers() {
  // Derive the identifiers from our model list, as upstream's implementation
  // reports the models we remove. Autoplay shares kMediaStream's identifier,
  // so skip duplicates.
  std::vector<ui::ElementIdentifier> result;
  for (const auto& model : GenerateContentSettingImageModels()) {
    const ui::ElementIdentifier identifier = model->GetElementIdentifier();
    if (!std::ranges::contains(result, identifier)) {
      result.push_back(identifier);
    }
  }
  return result;
}

// static
size_t ContentSettingImageModel::GetContentSettingImageModelIndexForTesting(
    ImageType image_type) {
  // Index into our model list, not the upstream one, as that's what the
  // location bar creates its views from.
  std::vector<std::unique_ptr<ContentSettingImageModel>> models =
      GenerateContentSettingImageModels();
  for (size_t i = 0; i < models.size(); ++i) {
    if (image_type == models[i]->image_type()) {
      return i;
    }
  }
  NOTREACHED();
}

void ContentSettingImageModel::GetIconFromType(
    ContentSettingsType type,
    bool blocked,
    raw_ptr<const gfx::VectorIcon>* icon,
    raw_ptr<const gfx::VectorIcon>* badge) {
  if (type == ContentSettingsType::AUTOPLAY) {
    *badge = (blocked ? &vector_icons::kBlockedBadgeCustomIcon
                      : &gfx::VectorIcon::EmptyIcon());
    *icon = &kAutoplayStatusIcon;
  } else {
    ::GetIconFromType(type, blocked, icon, badge);
  }
}
