// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_METRICS_RECORDER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_METRICS_RECORDER_H_

// Scrubs out histogramming calls
#define RecordIconShown() \
  RecordIconShown() {}    \
  void RecordIconShown_Chromium()
#define RecordChipShown() \
  RecordChipShown() {}    \
  void RecordChipShown_Chromium()
#define RecordIconClick() \
  RecordIconClick() {}    \
  void RecordIconClick_Chromium()
#define RecordChipClick() \
  RecordChipClick() {}    \
  void RecordChipClick_Chromium()

#include <chrome/browser/ui/views/page_action/page_action_metrics_recorder.h>  // IWYU pragma: export

#undef RecordChipClick
#undef RecordIconClick
#undef RecordChipShown
#undef RecordIconShown

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_METRICS_RECORDER_H_
