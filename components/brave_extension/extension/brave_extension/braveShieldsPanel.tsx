/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

import Theme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'

import { Provider } from 'react-redux'
import { Store } from 'react-chrome-redux'
import BraveShields from './containers/braveShields'
require('./assets/fonts/muli.css')
require('./assets/fonts/poppins.css')

chrome.storage.local.get('state', (obj) => {
  const store: any = new Store({
    portName: 'BRAVE'
  })

  store.ready()
    .then(() => {
      const mountNode: HTMLElement | null = document.querySelector('#root')
      ReactDOM.render(
        <Provider store={store}>
          <ThemeProvider theme={Theme}>
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
