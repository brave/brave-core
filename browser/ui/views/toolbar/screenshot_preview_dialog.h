// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_PREVIEW_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_PREVIEW_DIALOG_H_

#include <cstdint>
#include <vector>

#include "base/functional/callback_forward.h"
#include "ui/gfx/native_ui_types.h"

namespace screenshot {

// Shows a modal dialog previewing the captured `png` in a scroll view,
// taking ownership of it for the dialog's lifetime. Invokes `on_download`
// with `png` handed back if the user clicks the Download button, or
// `on_cancel` if they dismiss the dialog (Esc or closing the window).
// Exactly one of the two is run, exactly once.
void ShowScreenshotPreviewDialog(
    gfx::NativeWindow parent,
    std::vector<uint8_t> png,
    base::OnceCallback<void(std::vector<uint8_t>)> on_download,
    base::OnceClosure on_cancel);

}  // namespace screenshot

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_PREVIEW_DIALOG_H_
