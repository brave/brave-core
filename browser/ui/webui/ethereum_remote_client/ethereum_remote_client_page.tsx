// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'

import 'emptykit.css'

import LegacyApp from './components/legacy_app'
import { loadTimeData } from '../../../../components/common/loadTimeData'
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
  initLocale(loadTimeData.data_)

  render(
    <BraveCoreThemeProvider>
      <LegacyApp />
    </BraveCoreThemeProvider>,
    document.getElementById('root')
  )
}

document.addEventListener('DOMContentLoaded', initialize)
