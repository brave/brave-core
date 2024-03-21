/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PRINTING_RENDERER_PRINT_RENDER_FRAME_HELPER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PRINTING_RENDERER_PRINT_RENDER_FRAME_HELPER_H_

#include "components/printing/common/print.mojom.h"

#define SetPrintPreviewUI                                            \
  SetPrintPreviewUI_ChromiumImpl(                                    \
      mojo::PendingAssociatedRemote<mojom::PrintPreviewUI> preview); \
  void SetPrintPreviewUI
#include "src/components/printing/renderer/print_render_frame_helper.h"  // IWYU pragma: export
#undef SetPrintPreviewUI

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PRINTING_RENDERER_PRINT_RENDER_FRAME_HELPER_H_
