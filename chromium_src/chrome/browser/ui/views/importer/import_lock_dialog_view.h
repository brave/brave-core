/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_IMPORTER_IMPORT_LOCK_DIALOG_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_IMPORTER_IMPORT_LOCK_DIALOG_VIEW_H_

#include "ui/views/window/dialog_delegate.h"

#define Show(...)                                             \
  Show(__VA_ARGS__);                                          \
  static void Show(gfx::NativeView parent_view, __VA_ARGS__); \
  ui::mojom::ModalType GetModalType() const override

#include "src/chrome/browser/ui/views/importer/import_lock_dialog_view.h"  // IWYU pragma: export

#undef Show

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_IMPORTER_IMPORT_LOCK_DIALOG_VIEW_H_
