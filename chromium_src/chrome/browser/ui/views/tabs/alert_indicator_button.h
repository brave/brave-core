// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_ALERT_INDICATOR_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_ALERT_INDICATOR_BUTTON_H_

#include "ui/base/metadata/metadata_header_macros.h"

class AlertIndicatorButton;
#define AlertIndicatorButton AlertIndicatorButtonBase
#define GetTab     \
  GetTab_Unused(); \
                   \
 protected:        \
  Tab* GetTab

#define UpdateEnabledForMuteToggle virtual UpdateEnabledForMuteToggle
#include "src/chrome/browser/ui/views/tabs/alert_indicator_button.h"  // IWYU pragma: export
#undef UpdateEnabledForMuteToggle
#undef GetTab
#undef AlertIndicatorButton

class AlertIndicatorButton : public AlertIndicatorButtonBase {
  METADATA_HEADER(AlertIndicatorButton, AlertIndicatorButtonBase)
 public:
  using AlertIndicatorButtonBase::AlertIndicatorButtonBase;
  void UpdateEnabledForMuteToggle() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_ALERT_INDICATOR_BUTTON_H_
