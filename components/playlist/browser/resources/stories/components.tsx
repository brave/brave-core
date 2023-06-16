// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { withKnobs } from '@storybook/addon-knobs'
import { BrowserRouter } from 'react-router-dom'

import '@brave/leo/tokens/css/variables.css'
import BraveCoreThemeProvider from '../../../../common/BraveCoreThemeProvider'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'

import PlaylistsCatalog from '../components/playlistsCatalog'
import store from '../store'

import { mockData } from './mockData'

export default {
  title: 'Playlist/Components',
  decorators: [
    (Story: any) => <Provider store={store}>
    <BrowserRouter>
      <BraveCoreThemeProvider
        dark={DarkTheme}
        light={Theme}>
          <Story />
      </BraveCoreThemeProvider>
    </BrowserRouter>
    </Provider>,
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

export const Catalog = () => {
    return <PlaylistsCatalog playlists={mockData}></PlaylistsCatalog>
}
