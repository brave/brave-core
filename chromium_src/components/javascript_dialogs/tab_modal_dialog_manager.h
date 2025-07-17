/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_JAVASCRIPT_DIALOGS_TAB_MODAL_DIALOG_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_JAVASCRIPT_DIALOGS_TAB_MODAL_DIALOG_MANAGER_H_

#define BrowserActiveStateChanged \
  OnTabActiveStateChanged();      \
  void BrowserActiveStateChanged

#include "src/components/javascript_dialogs/tab_modal_dialog_manager.h"  // IWYU pragma: export

#undef BrowserActiveStateChanged

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_JAVASCRIPT_DIALOGS_TAB_MODAL_DIALOG_MANAGER_H_
