/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PLUGIN_DATA_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PLUGIN_DATA_H_

#define AddMimeType                                                          \
  SetName(const String& new_name) { name_ = new_name; }                      \
  void SetFilename(const String& new_filename) { filename_ = new_filename; } \
  void AddMimeType

#include "../../../../../../../third_party/blink/renderer/core/page/plugin_data.h"

#undef AddMimeType

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PLUGIN_DATA_H_
