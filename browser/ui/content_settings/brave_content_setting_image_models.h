/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_CONTENT_SETTING_IMAGE_MODELS_H_
#define BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_CONTENT_SETTING_IMAGE_MODELS_H_

#include <memory>
#include <vector>

class ContentSettingImageModel;

void BraveGenerateContentSettingImageModels(
    std::vector<std::unique_ptr<ContentSettingImageModel>>*);

#endif  // BRAVE_BROWSER_UI_CONTENT_SETTINGS_BRAVE_CONTENT_SETTING_IMAGE_MODELS_H_
