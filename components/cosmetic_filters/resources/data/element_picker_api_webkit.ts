// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ElementPickerAPI } from './element_picker_api'

const postNativeMessage = $((action, data) => {
  return $.postNativeMessage(messageHandler, {
    'securityToken': SECURITY_TOKEN,
    'data': {
      windowOrigin: $.windowOrigin,
      action: action,
      data: data,
    },
  })
})

export class WebKitElementPickerAPI implements ElementPickerAPI {
  cosmeticFilterCreate(selector: string) {
    postNativeMessage('addCosmeticFilter', selector)
  }

  cosmeticFilterManage() {
    postNativeMessage('manageCustomFilters', undefined)
  }

  getElementPickerThemeInfo(
    callback: (isDarkModeEnabled: boolean, bgcolor: number) => void,
  ) {
    postNativeMessage('elementPickerThemeInfo', undefined)
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
    postNativeMessage('localizedTexts', undefined)
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
    return 'ios'
  }
}
