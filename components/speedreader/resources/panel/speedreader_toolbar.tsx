// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'
import { ToolbarWrapperStyles } from './style'

import { loadTimeData } from '../../../common/loadTimeData'
import BraveCoreThemeProvider from '../../../common/BraveCoreThemeProvider'
import Toolbar from './components/toolbar'

import { setIconBasePath } from '@brave/leo/react/icon'
import '@brave/leo/tokens/css/variables.css'

setIconBasePath('//resources/brave-icons')

function App () {
  return (
    <BraveCoreThemeProvider>
      <ToolbarWrapperStyles>
        <Toolbar />
      </ToolbarWrapperStyles>
    </BraveCoreThemeProvider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
