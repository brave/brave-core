/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/webui/print_preview/print_preview_ui.cc"

namespace printing {

// static
base::IDMap<mojom::PrintPreviewUI*>& PrintPreviewUI::GetPrintPreviewUIIdMap() {
  return g_print_preview_ui_id_map.Get();
}

// static
PrintPreviewRequestIdMap& PrintPreviewUI::GetPrintPreviewUIRequestIdMap() {
  return GetPrintPreviewRequestIdMap();
}

}  // namespace printing
