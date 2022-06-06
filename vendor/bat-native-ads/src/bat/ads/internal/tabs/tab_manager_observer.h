/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_MANAGER_OBSERVER_H_

#include <cstdint>

#include "base/observer_list_types.h"

namespace ads {

class TabManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when the tab specfied by |id| changes focus.
  virtual void OnTabDidChangeFocus(const int32_t id) {}

  // Invoked when the tab specified by |id| is updated.
  virtual void OnTabDidChange(const int32_t id) {}

  // Invoked when a new tab is opened for the specified |id|.
  virtual void OnDidOpenNewTab(const int32_t id) {}

  // Invoked when a tab is closed.
  virtual void OnDidCloseTab(const int32_t id) {}

  // Invoked when media starts playing in a tab specified by |id|.
  virtual void OnTabDidStartPlayingMedia(const int32_t id) {}

  // Invoked when media stops playing in a tab specified by |id|.
  virtual void OnTabDidStopPlayingMedia(const int32_t id) {}

 protected:
  ~TabManagerObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_MANAGER_OBSERVER_H_
