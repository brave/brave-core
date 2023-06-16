// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { withKnobs } from '@storybook/addon-knobs'

import BraveCoreThemeProvider from '../../../../common/BraveCoreThemeProvider'
import store from '../store'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import { getAllActions } from '../api/getAllActions'
import { mockData } from './mockData'
import { BrowserRouter } from 'react-router-dom'
import App from '../components/app.v1'

export default {
  title: 'Playlist/Panel',
  decorators: [
    (Story: any) => 
    <BrowserRouter>
      <Provider store={store}>
        <BraveCoreThemeProvider dark={DarkTheme} light={Theme}>
            <Story />
        </BraveCoreThemeProvider>
      </Provider>
    </BrowserRouter>,
    (Story: any) => (
      <div
        style={{
          fontFamily: 'Poppins',
          width: '100%',
          minHeight: '100vh',
        }}
      >
        <Story />
      </div>
    ),
    withKnobs
  ]
}

export const AppV1 = () => {
  return (<App />)
}

getAllActions().playlistLoaded(mockData)
