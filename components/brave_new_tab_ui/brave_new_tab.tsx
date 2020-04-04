// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../common/BraveCoreThemeProvider'
import { wireApiEventsToStore } from './apiEventsToStore'

// Components
import App from './containers/app'

// Utils
import store from './store'
import 'emptykit.css'

// Fonts
import '../../ui/webui/resources/fonts/poppins.css'
import '../../ui/webui/resources/fonts/muli.css'
import '../../ui/webui/resources/fonts/crypto_fonts.css'
import '../../ui/webui/resources/css/crypto_styles.css'

function initialize () {
  console.timeStamp('loaded')
  // Get rendering going
  new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve))
  .then((themeType: chrome.braveTheme.ThemeType) => {
    render(
      <Provider store={store}>
        <BraveCoreThemeProvider
          initialThemeType={themeType}
          dark={DarkTheme}
          light={Theme}
        >
          <App />
        </BraveCoreThemeProvider>
      </Provider>,
      document.getElementById('root'),
      () => console.timeStamp('first react render'))
  })
  .catch((error) => {
    console.error('Problem mounting brave new tab', error)
  })
  window.i18nTemplate.process(window.document, window.loadTimeData)
}

console.timeStamp('JS start')

// Get store data going
wireApiEventsToStore()

// Perform DOM-dependent initialization when ready
document.addEventListener('DOMContentLoaded', initialize)
