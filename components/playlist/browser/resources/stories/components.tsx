// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { withKnobs } from '@storybook/addon-knobs'
import { BrowserRouter, MemoryRouter } from 'react-router-dom'

import '@brave/leo/tokens/css/variables.css'

import PlaylistsCatalog from '../components/playlistsCatalog'
import store from '../store'
import { mockData } from './mockData'
import { getAllActions } from '../api/getAllActions'

export default {
  title: 'Playlist/Components',
  decorators: [
    (Story: any) => (
      <MemoryRouter initialEntries={['/']}>
        <BrowserRouter>
          <Provider store={store}>
            <Story />
          </Provider>
        </BrowserRouter>
      </MemoryRouter>
    ),
    (Story: any) => (
      <div
        style={{
          fontFamily: 'Poppins',
          width: '100%',
          minHeight: '100vh'
        }}
      >
        <Story />
      </div>
    ),
    withKnobs
  ]
}

getAllActions().playlistLoaded(mockData)

export const Catalog = () => {
  return <PlaylistsCatalog />
}
