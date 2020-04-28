/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

import { ThemeProvider } from 'brave-ui/theme'
import Theme from 'brave-ui/theme/brave-default'
import { initLocale } from 'brave-ui/helpers'

import { App } from './components/app'
import { createHost } from './host'
import { LocaleContext } from '../ui/components/checkout/localeContext'

import 'emptykit.css'
import '../../../../ui/webui/resources/fonts/muli.css'
import '../../../../ui/webui/resources/fonts/poppins.css'

document.addEventListener('DOMContentLoaded', () => {
  // TODO(zenparsing): Error handling
  const host = createHost()
  const localeData = { get: (key: string) => host.getLocaleString(key) }

  // Required by brave-ui toggle component
  if (self.loadTimeData.data_) {
    initLocale(self.loadTimeData.data_)
  }

  document.body.addEventListener('keyup', (evt) => {
    if (evt.key.toLowerCase() === 'escape') {
      host.closeDialog()
    }
  })

  function Root () {
    return (
      <LocaleContext.Provider value={localeData}>
        <ThemeProvider theme={Theme}>
          <App host={host} exchangeCurrency='USD' />
        </ThemeProvider>
      </LocaleContext.Provider>
    )
  }

  ReactDOM.render(<Root />, document.getElementById('root'))
})
