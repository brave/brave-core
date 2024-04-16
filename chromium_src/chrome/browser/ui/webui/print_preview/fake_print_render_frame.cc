/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/webui/print_preview/fake_print_render_frame.cc"

namespace printing {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void FakePrintRenderFrame::SetIsPrintPreviewExtraction(bool value) {}
#endif
}  // namespace printing
