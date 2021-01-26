/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_ITEM_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_ITEM_VIEW_H_

#define BRAVE_DOWNLOAD_DOWNLOAD_ITEM_VIEW_H_ \
 private:                                    \
  friend class BraveDownloadItemView;        \
                                             \
 public:                                     \
 protected:                                  \
  bool IsShowingWarningDialog() const;

#define SetMode virtual SetMode
#include "../../../../../../../chrome/browser/ui/views/download/download_item_view.h"
#undef SetMode
#undef BRAVE_DOWNLOAD_DOWNLOAD_ITEM_VIEW_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_ITEM_VIEW_H_
