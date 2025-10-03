// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { withKnobs } from '@storybook/addon-knobs'

import store from '../store'
import { getPlaylistActions } from '../api/getPlaylistActions'
import { mockData } from './mockData'
import { MemoryRouter } from 'react-router-dom'
import App from '../components/app.v1'

export default {
  title: 'Playlist/Panel',
  decorators: [
    (Story: any) => (
      <MemoryRouter initialEntries={['/']}>
        <Provider store={store}>
          <Story />
        </Provider>
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
  ],
  component: App
}

export const AppV1 = {
  render: () => {
    return <App />
  }
}

getPlaylistActions().playlistLoaded(mockData)
