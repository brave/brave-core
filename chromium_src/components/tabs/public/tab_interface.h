// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_INTERFACE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_INTERFACE_H_

#include "build/build_config.h"

// Add method declarations to TabInterface to get opener of this tab.
#if !BUILDFLAG(IS_ANDROID)
#define IsInNormalWindow                 \
  IsInNormalWindow() const = 0;          \
  virtual TabInterface* GetOpener() = 0; \
  virtual const TabInterface* GetOpener
#endif  // !BUILDFLAG(IS_ANDROID)

#include <components/tabs/public/tab_interface.h>  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef IsInNormalWindow
#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_INTERFACE_H_
