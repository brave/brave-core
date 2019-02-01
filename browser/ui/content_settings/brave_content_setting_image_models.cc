/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_content_setting_image_models.h"

#include "brave/browser/ui/content_settings/brave_autoplay_blocked_image_model.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) || BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "brave/browser/ui/content_settings/brave_widevine_blocked_image_model.h"
#endif

void BraveGenerateContentSettingImageModels(
    std::vector<std::unique_ptr<ContentSettingImageModel>>* result) {
  // Assigned to ref to make it clear. Otherwise, we need to use (*result)[i]
  // for deferencing its element.
  std::vector<std::unique_ptr<ContentSettingImageModel>>& result_ref = *result;
  // Remove the cookies content setting image model
  // https://github.com/brave/brave-browser/issues/1197
  // TODO(iefremov): This changes break internal image models ordering which is
  // based on enum values. This breaks tests and probably should be fixed
  // (by adding more diff of course).
  for (size_t i = 0; i < result_ref.size(); i++) {
    if (result_ref[i]->image_type() ==
        ContentSettingImageModel::ImageType::COOKIES) {
      result_ref.erase(result_ref.begin() + i);
      break;
    }
  }

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) || BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  result_ref.push_back(std::make_unique<BraveWidevineBlockedImageModel>(
      BraveWidevineBlockedImageModel::ImageType::PLUGINS,
      CONTENT_SETTINGS_TYPE_PLUGINS));
#endif

  result_ref.push_back(std::make_unique<BraveAutoplayBlockedImageModel>());
}
