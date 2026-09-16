/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_IMAGE_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_IMAGE_MODEL_H_

#define GenerateContentSettingImageModels                       \
  GenerateContentSettingImageModels_ChromiumImpl();             \
  static std::vector<std::unique_ptr<ContentSettingImageModel>> \
      GenerateContentSettingImageModels

#define GetContentSettingImageModelIndexForTesting         \
  GetContentSettingImageModelIndexForTesting_ChromiumImpl( \
      ImageType image_type);                               \
  static size_t GetContentSettingImageModelIndexForTesting

#define GetAllElementIdentifiers           \
  GetAllElementIdentifiers_ChromiumImpl(); \
  static std::vector<ui::ElementIdentifier> GetAllElementIdentifiers

#define SetFramebustBlockedIcon                           \
  GetIconFromType(ContentSettingsType type, bool blocked, \
                  raw_ptr<const gfx::VectorIcon>* icon,   \
                  raw_ptr<const gfx::VectorIcon>* badge); \
  void SetFramebustBlockedIcon

#include <chrome/browser/ui/content_settings/content_setting_image_model.h>  // IWYU pragma: export
#undef SetFramebustBlockedIcon
#undef GetAllElementIdentifiers
#undef GetContentSettingImageModelIndexForTesting
#undef GenerateContentSettingImageModels

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_IMAGE_MODEL_H_
