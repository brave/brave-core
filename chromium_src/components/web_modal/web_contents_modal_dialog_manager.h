/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_WEB_MODAL_WEB_CONTENTS_MODAL_DIALOG_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_WEB_MODAL_WEB_CONTENTS_MODAL_DIALOG_MANAGER_H_

class SplitViewWithTabDialogBrowserTest;

#define FocusTopmostDialog                          \
  OnTabActiveStateChanged();                        \
  friend class ::SplitViewWithTabDialogBrowserTest; \
  void FocusTopmostDialog

#include <components/web_modal/web_contents_modal_dialog_manager.h>  // IWYU pragma: export

#undef FocusTopmostDialog

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_WEB_MODAL_WEB_CONTENTS_MODAL_DIALOG_MANAGER_H_
