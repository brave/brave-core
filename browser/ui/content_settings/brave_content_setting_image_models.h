/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_CONTENT_SETTING_IMAGE_MODELS_H_
#define BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_CONTENT_SETTING_IMAGE_MODELS_H_

#include <array>
#include <memory>
#include <vector>

#include "chrome/browser/ui/content_settings/content_setting_image_model.h"

// Image models removed so that their related icons don't appear in the URL
// bar:
// - Cookies (https://github.com/brave/brave-browser/issues/1197)
// - JavaScript (https://github.com/brave/brave-browser/issues/199)
// - StorageAccess (https://github.com/brave/brave-browser/issues/56810)
inline constexpr auto kBraveRemovedContentSettingImageTypes =
    std::to_array<ContentSettingImageModel::ImageType>({
        ContentSettingImageModel::ImageType::kCookies,
        ContentSettingImageModel::ImageType::kJavaScript,
        ContentSettingImageModel::ImageType::kStorageAccess,
    });

void BraveGenerateContentSettingImageModels(
    std::vector<std::unique_ptr<ContentSettingImageModel>>*);

#endif  // BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_CONTENT_SETTING_IMAGE_MODELS_H_
