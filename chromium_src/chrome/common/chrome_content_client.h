/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_CONTENT_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_CONTENT_CLIENT_H_

#include "content/public/common/content_client.h"

#define AddPlugins                                                       \
  AddPlugins(std::vector<content::ContentPluginInfo>* plugins) override; \
  void AddPlugins_ChromiumImpl

#include "src/chrome/common/chrome_content_client.h"  // IWYU pragma: export

#undef AddPlugins

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_CONTENT_CLIENT_H_
