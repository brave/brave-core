/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_TAB_MODAL_DIALOG_MANAGER_DELEGATE_DESKTOP_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_TAB_MODAL_DIALOG_MANAGER_DELEGATE_DESKTOP_H_

#include "components/javascript_dialogs/tab_modal_dialog_manager_delegate.h"

#define CreateNewDialog                                                       \
  CreateNewDialog_ChromiumImpl(                                               \
      content::WebContents* alerting_web_contents,                            \
      const std::u16string& title, content::JavaScriptDialogType dialog_type, \
      const std::u16string& message_text,                                     \
      const std::u16string& default_prompt_text,                              \
      content::JavaScriptDialogManager::DialogClosedCallback dialog_callback, \
      base::OnceClosure dialog_closed_callback);                              \
  base::WeakPtr<javascript_dialogs::TabModalDialogView> CreateNewDialog

#include "src/chrome/browser/ui/javascript_dialogs/javascript_tab_modal_dialog_manager_delegate_desktop.h"  // IWYU pragma: export

#undef CreateNewDialog

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_TAB_MODAL_DIALOG_MANAGER_DELEGATE_DESKTOP_H_
