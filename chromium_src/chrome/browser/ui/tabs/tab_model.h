// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_

#include "components/tabs/public/tab_interface.h"

#if !BUILDFLAG(IS_ANDROID)
#define IsInNormalWindow              \
  IsInNormalWindow() const override;  \
  TabInterface* GetOpener() override; \
  const TabInterface* GetOpener
#endif  // !BUILDFLAG(IS_ANDROID)

#include <chrome/browser/ui/tabs/tab_model.h>  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef IsInNormalWindow
#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_
