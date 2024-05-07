/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/printing/renderer/print_render_frame_helper.h"

#define PrintRenderFrameHelper PrintRenderFrameHelper_ChromiumImpl
#include "src/components/printing/renderer/print_render_frame_helper.cc"
#undef PrintRenderFrameHelper

namespace printing {

PrintRenderFrameHelper::PrintRenderFrameHelper(
    content::RenderFrame* render_frame,
    std::unique_ptr<Delegate> delegate)
    : PrintRenderFrameHelper_ChromiumImpl(render_frame, std::move(delegate)) {}

PrintRenderFrameHelper::~PrintRenderFrameHelper() = default;

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void PrintRenderFrameHelper::SetPrintPreviewUI(
    mojo::PendingAssociatedRemote<mojom::PrintPreviewUI> preview) {
  preview_ui_.reset();
  PrintRenderFrameHelper_ChromiumImpl::SetPrintPreviewUI(std::move(preview));
}

void PrintRenderFrameHelper::InitiatePrintPreview(
#if BUILDFLAG(IS_CHROMEOS_ASH)
    mojo::PendingAssociatedRemote<mojom::PrintRenderer> print_renderer,
#endif
    bool has_selection) {
  if (!is_print_preview_extraction_) {
    PrintRenderFrameHelper_ChromiumImpl::InitiatePrintPreview(
#if BUILDFLAG(IS_CHROMEOS_ASH)
        std::move(print_renderer),
#endif
        has_selection);
    return;
  }

  ScopedIPC scoped_ipc(weak_ptr_factory_.GetWeakPtr());
  if (ipc_nesting_level_ > kAllowedIpcDepthForPrint) {
    return;
  }

  if (print_in_progress_) {
    return;
  }

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  // If we are printing a frame with an internal PDF plugin element, find the
  // plugin node and print that instead.
  auto plugin = delegate_->GetPdfElement(frame);
  if (!plugin.IsNull()) {
    print_preview_context_.InitWithNode(plugin);
  } else {
    print_preview_context_.InitWithFrame(frame);
    print_preview_context_.DispatchBeforePrintEvent(
        weak_ptr_factory_.GetWeakPtr());
  }
  // Print Preview resets `print_in_progress_` when the dialog closes.
}

void PrintRenderFrameHelper::SetIsPrintPreviewExtraction(bool value) {
  is_print_preview_extraction_ = value;
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

}  // namespace printing
