/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'

import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

// Containers
import App from './containers/App'

// Utils
import store from './store'
import { setDialogArgs } from './slices/webcompatreporter.slice'
import {
  getDialogArgs,
  setViewPortChangeListener,
  ViewPortSizeChangedObject
} from './browser_proxy'

function loadDialogArgs() {
  const dialogArgsRaw = getDialogArgs()
  let dialogArgs
  try {
    dialogArgs = JSON.parse(dialogArgsRaw)
  } catch (e) {
    console.error('Error parsing incoming dialog args: ', dialogArgsRaw, e)
  }

  store.dispatch(setDialogArgs(dialogArgs))
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

  render(
    <Provider store={store}>
      <BraveCoreThemeProvider
        dark={DarkTheme}
        light={Theme}
      >
        <App />
      </BraveCoreThemeProvider>
    </Provider>,
    document.getElementById('root')
  )
}

document.addEventListener('DOMContentLoaded', initialize)
