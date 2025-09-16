// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_MOCK_TAB_INTERFACE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_MOCK_TAB_INTERFACE_H_

#include "build/build_config.h"
#include "components/tabs/public/tab_interface.h"

#if !BUILDFLAG(IS_ANDROID)
// Replace MockTabInterface with our own version that extends methods.
#define MockTabInterface MockTabInterfaceChromium
#endif  // !BUILDFLAG(IS_ANDROID)

#include <components/tabs/public/mock_tab_interface.h>  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef MockTabInterface

namespace tabs {
class MockTabInterface : public MockTabInterfaceChromium {
 public:
  MockTabInterface();
  ~MockTabInterface() override;

  // TabInterface:
  MOCK_METHOD(TabInterface*, GetOpener, (), (override));
  MOCK_METHOD(const TabInterface*, GetOpener, (), (const override));
};

}  // namespace tabs
#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_MOCK_TAB_INTERFACE_H_
