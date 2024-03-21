/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PRINT_PREVIEW_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PRINT_PREVIEW_UI_H_

#include "base/containers/id_map.h"

#define ClearPreviewUIId                                                \
  ClearPreviewUIId_Unused();                                            \
  static base::IDMap<mojom::PrintPreviewUI*>& GetPrintPreviewUIIdMap(); \
  static base::flat_map<int, int>& GetPrintPreviewUIRequestIdMap();     \
  void ClearPreviewUIId

#include "src/chrome/browser/ui/webui/print_preview/print_preview_ui.h"  // IWYU pragma: export

#undef ClearPreviewUIId

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PRINT_PREVIEW_UI_H_
