/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_

#include "components/tabs/public/tab_interface.h"

// Extends to support getter and setter for partitioned tabs.
#define IsActivated                                                     \
  IsPartitionedTab() const override;                                    \
  void SetPartitionedTabVisualData(                                     \
      const std::optional<PartitionedTabVisualData>& data);             \
  std::optional<PartitionedTabVisualData> GetPartitionedTabVisualData() \
      const override;                                                   \
                                                                        \
 private:                                                               \
  std::optional<PartitionedTabVisualData> partitioned_tab_visual_data_; \
                                                                        \
 public:                                                                \
  bool IsActivated

#include "src/chrome/browser/ui/tabs/tab_model.h"  // IWYU pragma: export

#undef IsActivated

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_
