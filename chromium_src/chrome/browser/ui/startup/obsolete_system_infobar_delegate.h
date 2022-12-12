/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_

#include "components/infobars/core/confirm_infobar_delegate.h"

#define Create                                     \
  Create_UnUsed() {}                               \
  friend class BraveObsoleteSystemInfoBarDelegate; \
  static void Create

#include "src/chrome/browser/ui/startup/obsolete_system_infobar_delegate.h"

#undef Create

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_
