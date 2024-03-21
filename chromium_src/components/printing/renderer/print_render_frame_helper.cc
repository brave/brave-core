/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/printing/renderer/print_render_frame_helper.h"

#define SetPrintPreviewUI SetPrintPreviewUI_ChromiumImpl
#include "src/components/printing/renderer/print_render_frame_helper.cc"
#undef SetPrintPreviewUI

namespace printing {

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void PrintRenderFrameHelper::SetPrintPreviewUI(
    mojo::PendingAssociatedRemote<mojom::PrintPreviewUI> preview) {
  preview_ui_.reset();
  SetPrintPreviewUI_ChromiumImpl(std::move(preview));
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

}  // namespace printing
