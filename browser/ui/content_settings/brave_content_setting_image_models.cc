/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_content_setting_image_models.h"

#include "brave/browser/ui/content_settings/brave_widevine_blocked_image_model.h"
#include "brave/browser/ui/content_settings/brave_autoplay_blocked_image_model.h"

void BraveGenerateContentSettingImageModels(
    std::vector<std::unique_ptr<ContentSettingImageModel>>& result) {
  // Remove the cookies content setting image model
  // https://github.com/brave/brave-browser/issues/1197
  for (size_t i = 0; i < result.size(); i++) {
    if (result[i]->image_type() ==
        ContentSettingImageModel::ImageType::COOKIES) {
      result.erase(result.begin() + i);
      break;
    }
  }

  result.push_back(std::make_unique<BraveWidevineBlockedImageModel>(
      BraveWidevineBlockedImageModel::ImageType::PLUGINS,
      CONTENT_SETTINGS_TYPE_PLUGINS));

  result.push_back(std::make_unique<BraveAutoplayBlockedImageModel>());
}
