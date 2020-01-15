/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

// Components
import App from './components/app'

// Utils
import store from './store'
import * as adblockActions from './actions/adblock_actions'

window.cr.define('brave_adblock', function () {
  'use strict'

  function getCustomFilters () {
    const actions = bindActionCreators(adblockActions, store.dispatch.bind(store))
    actions.getCustomFilters()
  }

  function getRegionalLists () {
    const actions = bindActionCreators(adblockActions, store.dispatch.bind(store))
    actions.getRegionalLists()
  }

  function initialize () {
    getCustomFilters()
    getRegionalLists()
    render(
      <Provider store={store}>
        <App />
      </Provider>,
      document.getElementById('root'))
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  function onGetCustomFilters (customFilters: string) {
    const actions = bindActionCreators(adblockActions, store.dispatch.bind(store))
    actions.onGetCustomFilters(customFilters)
  }

  function onGetRegionalLists (regionalLists: AdBlock.FilterList[]) {
    const actions = bindActionCreators(adblockActions, store.dispatch.bind(store))
    actions.onGetRegionalLists(regionalLists)
  }

  function statsUpdated () {
    const actions = bindActionCreators(adblockActions, store.dispatch.bind(store))
    actions.statsUpdated()
  }

  return {
    initialize,
    onGetCustomFilters,
    onGetRegionalLists,
    statsUpdated
  }
})

document.addEventListener('DOMContentLoaded', window.brave_adblock.initialize)
