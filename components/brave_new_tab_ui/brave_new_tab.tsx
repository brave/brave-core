// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { Provider } from 'react-redux'
import { setIconBasePath } from '@brave/leo/react/icon'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import '../common/defaultTrustedTypesPolicy'
import BraveCoreThemeProvider from '../common/BraveCoreThemeProvider'
import * as topSitesAPI from './api/topSites'
import { init } from './actions/new_tab_actions'
import App from './containers/app'
import { wireApiEventsToStore } from './apiEventsToStore'
import store from './store'

setIconBasePath('chrome://resources/brave-icons')

// Let things handle 'init'
store.dispatch(init())

function initialize () {
  console.timeStamp('loaded')
  // Get rendering going
  new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve))
  .then((themeType: chrome.braveTheme.ThemeType) => {
    createRoot(document.getElementById('root')!).render(
      <Provider store={store}>
        <BraveCoreThemeProvider
          initialThemeType={themeType}
          dark={DarkTheme}
          light={Theme}
        >
          <App />
        </BraveCoreThemeProvider>
      </Provider>)
  })
  .catch((error) => {
    console.error('Problem mounting brave new tab', error)
  })
}

console.timeStamp('JS start')

// Get store data going
wireApiEventsToStore()

// Perform DOM-dependent initialization when ready
document.addEventListener('DOMContentLoaded', initialize)

// Update topsite tiles when NTP gets visible.
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'visible') {
    topSitesAPI.updateMostVisitedInfo()
  }
})
