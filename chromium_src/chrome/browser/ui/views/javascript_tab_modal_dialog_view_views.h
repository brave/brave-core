/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_JAVASCRIPT_TAB_MODAL_DIALOG_VIEW_VIEWS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_JAVASCRIPT_TAB_MODAL_DIALOG_VIEW_VIEWS_H_

#define JavaScriptTabModalDialogManagerDelegateDesktop \
  JavaScriptTabModalDialogManagerDelegateDesktop;      \
  friend class BraveJavaScriptTabModalDialogViewViews

#include "src/chrome/browser/ui/views/javascript_tab_modal_dialog_view_views.h"  // IWYU pragma: export

#undef JavaScriptTabModalDialogManagerDelegateDesktop

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_JAVASCRIPT_TAB_MODAL_DIALOG_VIEW_VIEWS_H_
