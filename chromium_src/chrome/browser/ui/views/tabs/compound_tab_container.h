/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMPOUND_TAB_CONTAINER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMPOUND_TAB_CONTAINER_H_

#define NumPinnedTabs                     \
  NumPinnedTabs_Unused() {                \
    return {};                            \
  }                                       \
  friend class BraveCompoundTabContainer; \
  int NumPinnedTabs
#define TransferTabBetweenContainers virtual TransferTabBetweenContainers
#define GetUnpinnedContainerIdealLeadingX \
  virtual GetUnpinnedContainerIdealLeadingX
#define GetTabContainerAt     \
  GetTabContainerAt_Unused(); \
  virtual TabContainer* GetTabContainerAt
#define ConvertUnpinnedContainerIdealBoundsToLocal \
  virtual ConvertUnpinnedContainerIdealBoundsToLocal

#include "src/chrome/browser/ui/views/tabs/compound_tab_container.h"  // IWYU pragma: export

#undef ConvertUnpinnedContainerIdealBoundsToLocal
#undef GetTabContainerAt
#undef GetUnpinnedContainerIdealLeadingX
#undef TransferTabBetweenContainers
#undef NumPinnedTabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMPOUND_TAB_CONTAINER_H_
