// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import Theme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'
import wireAPIEventsToStore from './apiEventsToStore'

// Components
import App from './components/app'

// Utils
import store from './store'
import 'emptykit.css'

// Fonts
import '../fonts/poppins.css'
import '../fonts/muli.css'

function initialize () {
  console.timeStamp('loaded')
  // Get rendering going
  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <App />
      </ThemeProvider>
    </Provider>,
    document.getElementById('root'),
    () => console.timeStamp('first react render')
  )
  window.i18nTemplate.process(window.document, window.loadTimeData)
}

console.timeStamp('JS start')

// Get store data going
wireAPIEventsToStore()

// Perform DOM-dependent initialization when ready
document.addEventListener('DOMContentLoaded', initialize)
