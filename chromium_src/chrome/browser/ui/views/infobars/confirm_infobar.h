/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INFOBARS_CONFIRM_INFOBAR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INFOBARS_CONFIRM_INFOBAR_H_

#define GetDelegate                            \
  Unused() { return nullptr; }                 \
  friend class BraveSyncAccountDeletedInfoBar; \
  friend class BraveConfirmInfoBar;            \
  ConfirmInfoBarDelegate* GetDelegate

#define NonLabelWidth virtual NonLabelWidth

#include "src/chrome/browser/ui/views/infobars/confirm_infobar.h"  // IWYU pragma: export

#undef NonLabelWidth
#undef GetDelegate

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INFOBARS_CONFIRM_INFOBAR_H_
