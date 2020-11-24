/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_content_setting_image_models.h"

#include "base/ranges/algorithm.h"
#include "brave/browser/ui/content_settings/brave_autoplay_blocked_image_model.h"
#include "third_party/widevine/cdm/buildflags.h"

using ImageType = ContentSettingImageModel::ImageType;

void BraveGenerateContentSettingImageModels(
    std::vector<std::unique_ptr<ContentSettingImageModel>>* result) {
  // Remove the cookies and javascript content setting image model
  // https://github.com/brave/brave-browser/issues/1197
  // https://github.com/brave/brave-browser/issues/199
  result->erase(
      base::ranges::remove_if(*result,
                              [](const auto& m) {
                                return m->image_type() == ImageType::COOKIES ||
                                       m->image_type() == ImageType::JAVASCRIPT;
                              }),
      result->end());

  result->push_back(std::make_unique<BraveAutoplayBlockedImageModel>());
}
