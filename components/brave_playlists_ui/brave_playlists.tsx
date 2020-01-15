/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

// Components
import App from './components/app'

// Theme
import BraveCoreThemeProvider from '../common/BraveCoreThemeProvider'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
// Utils
import store from './store'
import * as playlistsActions from './actions/playlists_actions'

window.cr.define('brave_playlists', function () {
  'use strict'

  function getCustomFilters () {
    const actions = bindActionCreators(playlistsActions, store.dispatch.bind(store))
    actions.getCustomFilters()
  }

  function getRegionalLists () {
    const actions = bindActionCreators(playlistsActions, store.dispatch.bind(store))
    actions.getRegionalLists()
  }

  function initialize () {
    getCustomFilters()
    getRegionalLists()
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
          document.getElementById('root'))
      })
      .catch((error) => {
        console.error('Problem mounting brave new tab', error)
      })
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  function onGetCustomFilters (customFilters: string) {
    const actions = bindActionCreators(playlistsActions, store.dispatch.bind(store))
    actions.onGetCustomFilters(customFilters)
  }

  function onGetRegionalLists (regionalLists: Playlists.FilterList[]) {
    const actions = bindActionCreators(playlistsActions, store.dispatch.bind(store))
    actions.onGetRegionalLists(regionalLists)
  }

  function statsUpdated () {
    const actions = bindActionCreators(playlistsActions, store.dispatch.bind(store))
    actions.statsUpdated()
  }

  return {
    initialize,
    onGetCustomFilters,
    onGetRegionalLists,
    statsUpdated
  }
})

document.addEventListener('DOMContentLoaded', window.brave_playlists.initialize)
