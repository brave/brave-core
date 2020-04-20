/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_

class BraveTabStripModel;

// Add virtual method to allow tab strip controllers to react upon MRU cycling
#define BRAVE_TAB_STRIP_MODEL_OBSERVER_H_ \
  virtual void StartMRUCycling(BraveTabStripModel* brave_tab_strip_model) {}

#include "../../../../../../chrome/browser/ui/tabs/tab_strip_model_observer.h"  // NOLINT
#undef BRAVE_TAB_STRIP_MODEL_OBSERVER_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
