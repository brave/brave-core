/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'
import shieldsDarkTheme from './theme/shields-dark'
import shieldsLightTheme from './theme/shields-light'
import { Provider } from 'react-redux'
import { Store } from 'webext-redux'
import BraveCoreThemeProvider from '../../../common/BraveCoreThemeProvider'
import BraveShields from './containers/app'
import { getViewPreferences } from './background/api/shieldsAPI'
require('../../../../ui/webui/resources/fonts/muli.css')
require('../../../../ui/webui/resources/fonts/poppins.css')

const store: any = new Store({
  portName: 'BRAVE'
})

Promise.all([
  store.ready(),
  new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve)),
  getViewPreferences()
])
.then((
  [ , themeType, settings ]:
  [
    undefined,
    chrome.braveTheme.ThemeType,
    chrome.braveShields.BraveShieldsViewPreferences
  ]
) => {
  const mountNode: HTMLElement | null = document.querySelector('#root')
  ReactDOM.render(
    <Provider store={store}>
      <BraveCoreThemeProvider
        initialThemeType={themeType}
        dark={shieldsDarkTheme}
        light={shieldsLightTheme}
      >
        <BraveShields settings={settings} />
      </BraveCoreThemeProvider>
    </Provider>,
    mountNode
  )
})
.catch((error: any) => {
  console.error('Problem mounting brave shields')
  console.error(error)
})
