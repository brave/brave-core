/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INCOGNITO_CLEAR_BROWSING_DATA_DIALOG_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INCOGNITO_CLEAR_BROWSING_DATA_DIALOG_H_

#define CloseDialog                                   \
  Unused_CloseDialog();                               \
  friend class BraveIncognitoClearBrowsingDataDialog; \
  static void CloseDialog

#include "src/chrome/browser/ui/views/incognito_clear_browsing_data_dialog.h"

#undef CloseDialog

class BraveIncognitoClearBrowsingDataDialog
    : public IncognitoClearBrowsingDataDialog {
 public:
  static void Show(views::View* anchor_view,
                   Profile* incognito_profile,
                   Type type);

  BraveIncognitoClearBrowsingDataDialog(views::View* anchor_view,
                                        Profile* incognito_profile,
                                        Type type);
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_INCOGNITO_CLEAR_BROWSING_DATA_DIALOG_H_
