// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import Theme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'
import * as dataFetchAPI from './api/dataFetch'
import * as preferencesAPI from './api/preferences'

// Components
import App from './containers/app'

// Utils
import store from './store'
import 'emptykit.css'

// Fonts
import '../fonts/poppins.css'
import '../fonts/muli.css'

async function initialize () {
  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <App />
      </ThemeProvider>
    </Provider>,
    document.getElementById('root')
  )
  window.i18nTemplate.process(window.document, window.loadTimeData)
  handleAPIEvents()
  await updatePreferences()
}

async function updatePreferences () {
  const preferences = await preferencesAPI.getPreferences()
  const actions = dataFetchAPI.getActions()
  actions.preferencesUpdated(preferences)
}

function updateStats () {
  const actions = dataFetchAPI.getActions()
  actions.statsUpdated()
}

function handleAPIEvents () {
  chrome.send('newTabPageInitialized', [])
  window.cr.addWebUIListener('stats-updated', updateStats)
  preferencesAPI.addChangeListener(updatePreferences)
}

document.addEventListener('DOMContentLoaded', initialize)
