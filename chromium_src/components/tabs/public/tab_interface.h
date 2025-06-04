// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_INTERFACE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_INTERFACE_H_

#include "brave/components/tabs/public/partitioned_tab_visual_data.h"

// Extend the TabInterface to include partitioned tab functionality.
#define IsActivated                               \
  IsPartitionedTab() const;                       \
  virtual std::optional<PartitionedTabVisualData> \
  GetPartitionedTabVisualData() const;            \
  virtual bool IsActivated

#include "src/components/tabs/public/tab_interface.h"  // IWYU pragma: export

#undef IsActivated

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_INTERFACE_H_
