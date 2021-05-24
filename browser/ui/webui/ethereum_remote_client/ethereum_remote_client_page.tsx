// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'

import 'emptykit.css'
import '../../../../ui/webui/resources/fonts/poppins.css'
import '../../../../ui/webui/resources/fonts/muli.css'

import LegacyApp from './components/legacy_app'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../../../../components/common/BraveCoreThemeProvider'

function initialize () {
  chrome.braveWallet.shouldPromptForSetup((prompt: boolean) => {
    if (!prompt) {
      chrome.braveWallet.loadUI(() => {
        window.location.href = 'chrome://wallet'
      })
      return
    }
    renderUI()
  })
}

function renderUI () {
  new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve))
  .then((themeType: chrome.braveTheme.ThemeType) => {
    window.i18nTemplate.process(window.document, window.loadTimeData)
    if (window.loadTimeData && window.loadTimeData.data_) {
      initLocale(window.loadTimeData.data_)
    }

    render(
      <BraveCoreThemeProvider
        initialThemeType={themeType}
        dark={DarkTheme}
        light={Theme}
      >
        <LegacyApp />
      </BraveCoreThemeProvider>,
      document.getElementById('root')
    )
  })
  .catch(({ message }) => {
    console.error(`Could not mount brave wallet: ${message}`)
  })
}

document.addEventListener('DOMContentLoaded', initialize)
