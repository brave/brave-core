/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INFOBARS_INFOBAR_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INFOBARS_INFOBAR_VIEW_H_

#define RecalculateHeight           \
  RecalculateHeight_Unused() {}     \
  friend class BraveConfirmInfoBar; \
  void RecalculateHeight

#define CloseButtonPressed virtual CloseButtonPressed

#include "src/chrome/browser/ui/views/infobars/infobar_view.h"  // IWYU pragma: export

#undef CloseButtonPressed
#undef RecalculateHeight

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INFOBARS_INFOBAR_VIEW_H_
