// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  sendTokenizedWebKitMessage,
  sendTokenizedWebKitMessageWithReply,
  messageHandlerName,
} from '//brave/ios/web/js_messaging/resources/utils.js'
import type { ElementPickerAPI } from '//brave/components/cosmetic_filters/resources/data/element_picker_api.js'

export class WebKitElementPickerAPI implements ElementPickerAPI {
  cosmeticFilterCreate(selector: string) {
    sendTokenizedWebKitMessage(messageHandlerName, {
      request_type: 'add_cosmetic_filter',
      data: { selector: selector },
    })
  }

  cosmeticFilterManage() {
    sendTokenizedWebKitMessage(messageHandlerName, {
      request_type: 'manage_custom_filters',
    })
  }

  getElementPickerThemeInfo(
    callback: (isDarkModeEnabled: boolean, bgcolor: number) => void,
  ) {
    sendTokenizedWebKitMessageWithReply(messageHandlerName, {
      request_type: 'theme_info',
    }).then((val: { isDarkModeEnabled: boolean; bgcolor: number }) => {
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
    sendTokenizedWebKitMessageWithReply(messageHandlerName, {
      request_type: 'localized_texts',
    }).then(
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
