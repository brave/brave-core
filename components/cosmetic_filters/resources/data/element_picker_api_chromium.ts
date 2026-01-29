// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ElementPickerAPI } from './element_picker_api'

export class ChromiumElementPickerAPI implements ElementPickerAPI {
  cosmeticFilterCreate(selector: string) {
    cf_worker.addSiteCosmeticFilter(selector)
  }

  cosmeticFilterManage() {
    cf_worker.manageCustomFilters()
  }

  getElementPickerThemeInfo(
    callback: (isDarkModeEnabled: boolean, bgcolor: number) => void,
  ) {
    cf_worker
      .getElementPickerThemeInfo()
      .then((val: { isDarkModeEnabled: boolean; bgcolor: number }) => {
        callback(val.isDarkModeEnabled, val.bgcolor)
      })
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
    cf_worker
      .getLocalizedTexts()
      .then(
        (val: {
          btnCreateDisabledText: string
          btnCreateEnabledText: string
          btnManageText: string
          btnShowRulesBoxText: string
          btnHideRulesBoxText: string
          btnQuitText: string
        }) => {
          callback(
            val.btnCreateDisabledText,
            val.btnCreateEnabledText,
            val.btnManageText,
            val.btnShowRulesBoxText,
            val.btnHideRulesBoxText,
            val.btnQuitText,
          )
        },
      )
  }

  getPlatform() {
    return cf_worker.getPlatform()
  }
}
