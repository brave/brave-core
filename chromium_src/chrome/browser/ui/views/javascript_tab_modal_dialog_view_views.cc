/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/javascript_dialogs/javascript_tab_modal_dialog_manager_delegate_desktop.h"

// CreateNewDialog is a function declared in
// JavaScriptTabModalDialogManagerDelegateDesktop. As we want our own factory
// function, replaces the function name. Our version will be in the
// brave_javascript_tab_modal_dialog_view_views.cc.
#define CreateNewDialog CreateNewDialog_ChromiumImpl

#include "src/chrome/browser/ui/views/javascript_tab_modal_dialog_view_views.cc"

#undef CreateNewDialog
