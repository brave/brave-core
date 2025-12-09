/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_SYSTEM_MENU_MODEL_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_SYSTEM_MENU_MODEL_DELEGATE_H_

#include "ui/menus/simple_menu_model.h"

#define IsCommandIdChecked                               \
  IsCommandIdChecked_ChromiumImpl(int command_id) const; \
  bool IsCommandIdChecked

#define GetLabelForCommandId(...)                       \
  GetLabelForCommandId_ChromiumImpl(__VA_ARGS__) const; \
  std::u16string GetLabelForCommandId(__VA_ARGS__)

#include <chrome/browser/ui/views/frame/system_menu_model_delegate.h>  // IWYU pragma: export

#undef GetLabelForCommandId
#undef IsCommandIdChecked

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_SYSTEM_MENU_MODEL_DELEGATE_H_
