/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_DATA_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_DATA_SOURCE_H_

#define AddBoolean                                                   \
  AddInteger(const std::string& name, int32_t value) = 0;            \
  virtual void AddDouble(const std::string& name, double value) = 0; \
  virtual void AddBoolean

#include <ios/web/public/webui/web_ui_ios_data_source.h>  // IWYU pragma: export

#undef AddBoolean

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_DATA_SOURCE_H_
