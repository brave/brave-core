/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/printing/print_view_manager.cc>

namespace printing {

void PrintViewManager::SetPrintPreviewRenderFrameHostForExtraction(
    content::RenderFrameHost* rfh) {
  // `SetPrintPreviewRenderFrameHost()` requires the print render frame
  // connection to already be established.
  GetPrintRenderFrame(rfh);
  SetPrintPreviewRenderFrameHost(rfh);
}

void PrintViewManager::ClearPrintPreviewRenderFrameHostForExtraction() {
  print_preview_rfh_ = nullptr;
}

}  // namespace printing
