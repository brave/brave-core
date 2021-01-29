/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_H_

#include "ui/views/window/dialog_delegate.h"

#define Show                   \
  ShowBrave(Profile* profile); \
  static void Show
#include "../../../../../../chrome/browser/ui/views/first_run_dialog.h"
#undef Show

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_H_
