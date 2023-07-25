// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { render } from 'react-dom'
import { Provider } from 'react-redux'

import store from './store'

import Player from './components/player'
import startReceivingAPIRequest from './playerApiSink'
import { setIconBasePath } from '@brave/leo/react/icon'
import '@brave/leo/tokens/css/variables.css'

function initialize () {
  render(
    <Provider store={store}>
      <Player />
    </Provider>,
    document.getElementById('root')
  )
}

startReceivingAPIRequest()
setIconBasePath('chrome-untrusted://resources/brave-icons')

document.addEventListener('DOMContentLoaded', initialize)
