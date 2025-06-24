// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TEST_MOCK_TAB_INTERFACE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TEST_MOCK_TAB_INTERFACE_H_

#define MockTabInterface MockTabInterfaceChromium

#include "src/chrome/browser/ui/tabs/test/mock_tab_interface.h"  // IWYU pragma: export

#undef MockTabInterface

namespace tabs {

class MockTabInterface : public MockTabInterfaceChromium {
 public:
  MockTabInterface();
  ~MockTabInterface() override;

  MOCK_METHOD(bool, IsPartitionedTab, (), (const, override));
  MOCK_METHOD(std::optional<PartitionedTabVisualData>,
              GetPartitionedTabVisualData,
              (),
              (const, override));
};

}  // namespace tabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TEST_MOCK_TAB_INTERFACE_H_
