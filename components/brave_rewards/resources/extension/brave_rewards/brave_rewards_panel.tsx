/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { Store } from 'webext-redux'

import Theme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'
import { initLocale } from 'brave-ui/helpers'
require('emptykit.css')
require('../../../../../ui/webui/resources/fonts/muli.css')
require('../../../../../ui/webui/resources/fonts/poppins.css')

import { LocaleContext } from '../../shared/lib/locale_context'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'

// Components
import App from './components/app'

// Utils
import { getMessage, getUIMessages } from './background/api/locale_api'

const store: Store<RewardsExtension.State> = new Store({
  portName: 'REWARDSPANEL'
})

const localeContext = {
  getString: (key: string) => getMessage(key)
}

initLocale(getUIMessages())

store.ready().then(
  () => {
    render(
      <LocaleContext.Provider value={localeContext}>
        <Provider store={store}>
          <ThemeProvider theme={Theme}>
            <WithThemeVariables>
              <App />
            </WithThemeVariables>
          </ThemeProvider>
        </Provider>
      </LocaleContext.Provider>,
      document.getElementById('root'))
  })
  .catch(() => {
    console.error('Problem mounting rewards panel')
  })
