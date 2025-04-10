/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import * as React from 'react'
 import { render } from 'react-dom'
 import Theme from 'brave-ui/theme/brave-default'
 import { setIconBasePath } from '@brave/leo/react/icon'

setIconBasePath('chrome://resources/brave-icons')

// Containers
import PsstDlgContainer from './containers/App'
import { ThemeProvider } from 'styled-components'

 function initialize () {
    console.log('[PSST] initialize')

      render(
          <ThemeProvider theme={Theme}>
               <PsstDlgContainer someProp={'T'} />
          </ThemeProvider>,
        document.getElementById('root')
      )  
 }

 document.addEventListener('DOMContentLoaded', initialize)