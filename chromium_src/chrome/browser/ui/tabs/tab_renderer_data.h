/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_RENDERER_DATA_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_RENDERER_DATA_H_

#define FromTabInModel                                          \
  FromTabInModel_ChromiumImpl(TabStripModel* model, int index); \
  static TabRendererData FromTabInModel

#include "../../../../../../chrome/browser/ui/tabs/tab_renderer_data.h"
#undef FromTabInModel

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_RENDERER_DATA_H_
