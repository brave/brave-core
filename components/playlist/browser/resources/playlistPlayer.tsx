// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { render } from 'react-dom'
import { Provider } from 'react-redux'

import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'

import store from './store'

import Player from './components/player'
import startReceiving from './playerApiSink'

function initialize () {
  render(
    <Provider store={store}>
      <BraveCoreThemeProvider
        dark={DarkTheme}
        light={Theme}
      >
        <Player />
      </BraveCoreThemeProvider>
    </Provider>,
    document.getElementById('root'))
}

startReceiving()

document.addEventListener('DOMContentLoaded', initialize)
