// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ElementPickerAPI } from './element_picker_api'

export class WebKitElementPickerAPI implements ElementPickerAPI {
  cosmeticFilterCreate(selector: string) {}

  cosmeticFilterManage() {}

  getElementPickerThemeInfo(
    callback: (isDarkModeEnabled: boolean, bgcolor: number) => void,
  ) {
    callback(true, 16777215) // white
  }

  getLocalizedTexts(
    callback: (
      btnCreateDisabledText: string,
      btnCreateEnabledText: string,
      btnManageText: string,
      btnShowRulesBoxText: string,
      btnHideRulesBoxText: string,
      btnQuitText: string,
    ) => void,
  ) {
    callback(
      'Select element you want to block',
      'Block Element',
      'manage',
      'show rules',
      'hide rules',
      'quit',
    )
  }

  getPlatform() {
    return 'ios'
  }
}
