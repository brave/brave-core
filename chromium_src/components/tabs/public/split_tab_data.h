// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_SPLIT_TAB_DATA_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_SPLIT_TAB_DATA_H_

#define ListTabs(...)            \
  ListTabs(__VA_ARGS__) const;   \
  bool linked() const {          \
    return linked_;              \
  }                              \
  void set_linked(bool linked) { \
    linked_ = linked;            \
  }                              \
                                 \
 private:                        \
  bool linked_ = false;          \
                                 \
 public:                         \
  void UnUsed()

#include <components/tabs/public/split_tab_data.h>  // IWYU pragma: export

#undef ListTabs
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_SPLIT_TAB_DATA_H_
