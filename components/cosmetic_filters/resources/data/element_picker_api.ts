// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export interface ElementPickerAPI {
  cosmeticFilterCreate: (selector: string) => void
  cosmeticFilterManage: () => void
  getElementPickerThemeInfo: (
    callback: (isDarkModeEnabled: boolean, bgcolor: number) => void,
  ) => void
  getLocalizedTexts: (
    callback: (
      btnCreateDisabledText: string,
      btnCreateEnabledText: string,
      btnManageText: string,
      btnShowRulesBoxText: string,
      btnHideRulesBoxText: string,
      btnQuitText: string,
    ) => void,
  ) => void
  getPlatform: () => string
}
