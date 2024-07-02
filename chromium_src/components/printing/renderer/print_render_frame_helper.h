/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PRINTING_RENDERER_PRINT_RENDER_FRAME_HELPER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PRINTING_RENDERER_PRINT_RENDER_FRAME_HELPER_H_

#include "components/printing/common/print.mojom.h"

namespace printing {
class PrintRenderFrameHelper;
using PrintRenderFrameHelper_BraveImpl = PrintRenderFrameHelper;
}  // namespace printing

#define PrintRenderFrameHelper PrintRenderFrameHelper_ChromiumImpl
#define SetupOnStopLoadingTimeout          \
  SetupOnStopLoadingTimeout_Unused();      \
  friend PrintRenderFrameHelper_BraveImpl; \
  void SetupOnStopLoadingTimeout
#include "src/components/printing/renderer/print_render_frame_helper.h"  // IWYU pragma: export
#undef SetupOnStopLoadingTimeout
#undef PrintRenderFrameHelper
#undef SetPrintPreviewUI

namespace printing {
class PrintRenderFrameHelper : public PrintRenderFrameHelper_ChromiumImpl {
 public:
  PrintRenderFrameHelper(content::RenderFrame* render_frame,
                         std::unique_ptr<Delegate> delegate);
  PrintRenderFrameHelper(const PrintRenderFrameHelper&) = delete;
  PrintRenderFrameHelper& operator=(const PrintRenderFrameHelper&) = delete;
  ~PrintRenderFrameHelper() override;

 private:
  // printing::mojom::PrintRenderFrame:
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  void SetPrintPreviewUI(
      mojo::PendingAssociatedRemote<mojom::PrintPreviewUI> preview) override;
  void InitiatePrintPreview(
#if BUILDFLAG(IS_CHROMEOS_ASH)
      mojo::PendingAssociatedRemote<mojom::PrintRenderer> print_renderer,
#endif
      bool has_selection) override;
  void SetIsPrintPreviewExtraction(bool value) override;
#endif

  bool is_print_preview_extraction_ = false;
};
}  // namespace printing

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PRINTING_RENDERER_PRINT_RENDER_FRAME_HELPER_H_
