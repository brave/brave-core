// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_

#define OnImageLoaded             \
  OnImageLoaded_Chromium() {}     \
  friend class PaddedImageSource; \
  virtual void OnImageLoaded

#include "src/chrome/browser/ui/webui/sanitized_image_source.h"  // IWYU pragma: export
#undef OnImageLoaded

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_
