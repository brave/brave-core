/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

import shieldsDarkTheme from 'brave-ui/theme/shields-dark'
import shieldsLightTheme from 'brave-ui/theme/shields-light'
import { ThemeProvider } from 'brave-ui/theme'
import { ThemeType } from './types/other/theme'

import { Provider } from 'react-redux'
import { Store } from 'react-chrome-redux'
import BraveShields from './containers/braveShields'
require('../../../fonts/muli.css')
require('../../../fonts/poppins.css')

chrome.storage.local.get('state', (obj) => {
  const store: any = new Store({
    portName: 'BRAVE'
  })
  store.ready()
    .then(() => {
      const mountNode: HTMLElement | null = document.querySelector('#root')
      const theme: ThemeType = store.state.shieldsPanel.theme
      const getTheme = theme === 'Dark' ? shieldsDarkTheme : shieldsLightTheme
      ReactDOM.render(
        <Provider store={store}>
          <ThemeProvider theme={getTheme}>
            <BraveShields />
          </ThemeProvider>
        </Provider>,
        mountNode
      )
    })
    .catch(() => {
      console.error('Problem mounting brave shields')
    })
})
