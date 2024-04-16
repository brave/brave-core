/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_FAKE_PRINT_RENDER_FRAME_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_FAKE_PRINT_RENDER_FRAME_H_

#include "components/printing/common/print.mojom.h"

#define PrintForSystemDialog                        \
  SetIsPrintPreviewExtraction(bool value) override; \
  void PrintForSystemDialog
#include "src/chrome/browser/ui/webui/print_preview/fake_print_render_frame.h"  // IWYU pragma: export
#undef PrintForSystemDialog

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_FAKE_PRINT_RENDER_FRAME_H_
