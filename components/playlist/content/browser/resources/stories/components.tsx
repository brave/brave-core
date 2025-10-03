// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { withKnobs } from '@storybook/addon-knobs'
import { BrowserRouter, MemoryRouter } from 'react-router-dom'

import '@brave/leo/tokens/css/variables.css'

import store from '../store'
import { mockData } from './mockData'
import { getPlaylistActions } from '../api/getPlaylistActions'
import PlaylistsCatalog from '../components/playlistsCatalog'
import Player from '../components/player'
import { handlePlayerMessage } from '../playerApiSink'
import { types } from '../constants/player_types'
import ContextualMenuAnchorButton from '../components/contextualMenu'

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

getPlaylistActions().playlistLoaded(mockData)

export const Catalog = {
  render: () => {
    return <PlaylistsCatalog />
  }
}

export const VideoPlayer = {
  render: () => {
    handlePlayerMessage({
      actionType: types.PLAYLIST_ITEM_SELECTED,
      currentList: mockData.at(0)!,
      currentItem: mockData.at(0)!.items.at(0)!
    })
    return <Player />
  }
}

export const SampleContextualMenu = {
  render: () => {
    return (
      <div style={{ float: 'right' }}>
        <ContextualMenuAnchorButton
          items={[
            {
              name: 'test',
              iconName: 'trash',
              onClick: () => {
                window.alert('test')
              }
            },
            {
              name: 'looooooooooooooooooooong text this is really long',
              iconName: 'cloud-off',
              onClick: () => {
                window.alert('looooooong text')
              }
            }
          ]}
          visible={true}
        />
      </div>
    )
  }
}
