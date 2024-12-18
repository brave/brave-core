/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

// Containers
import App from './containers/App'

// Utils
import store from './store'
import * as webcompatReporterActions from './actions/webcompatreporter_actions'
import {
  getDialogArgs,
  setViewPortChangeListener,
  ViewPortSizeChangedObject,
} from './browser_proxy'

let actions: any

function getActions () {
  if (actions) {
    return actions
  }

  actions = bindActionCreators(webcompatReporterActions, store.dispatch.bind(store))
  return actions
}

function loadDialogArgs () {
  const dialogArgsRaw = getDialogArgs()
  let dialogArgs
  try {
    dialogArgs = JSON.parse(dialogArgsRaw)
  } catch (e) {
    console.error('Error parsing incoming dialog args: ', dialogArgsRaw, e)
  }

  getActions().setDialogArgs(dialogArgs)
}

function onViewPortSizeChanged(data: ViewPortSizeChangedObject) {
  const root = document.getElementById('root')
  if (root) {
    root.style.maxHeight = `${data.height}px`
  }
}

function initialize () {
  loadDialogArgs()

  setViewPortChangeListener(onViewPortSizeChanged)

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
      document.getElementById('root')
    )
  })
  .catch(error => {
    console.error('Problem mounting webcompat reporter modal', error)
  })
}

document.addEventListener('DOMContentLoaded', initialize)
