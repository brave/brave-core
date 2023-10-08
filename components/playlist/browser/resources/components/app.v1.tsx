/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Route, Switch } from 'react-router-dom'
import styled from 'styled-components'

// Components
import Header from './header'
import PlaylistsCatalog from './playlistsCatalog'
import PlaylistFolder from './playlistFolder'
import VideoFrame from './videoFrame'
import {
  PlaylistEditMode,
  useLastPlayerState,
  usePlaylistEditMode
} from '../reducers/states'
import { useHistorySynchronization } from '../playerEventSink'

const AppContainer = styled.div<{ isPlaylistPlayerPage: boolean }>`
  --header-height: ${({ isPlaylistPlayerPage }) =>
    isPlaylistPlayerPage ? '74px' : '56px'};
`

const HeaderWrapper = styled.header`
  position: sticky;
  width: 100%;
  height: var(--header-height);
  top: 0;
  z-index: 1;
`

export default function App () {
  useHistorySynchronization()

  const lastPlayerState = useLastPlayerState()
  const editMode = usePlaylistEditMode()

  return (
    <Route
      path={'/playlist/:playlistId'}
      children={({ match }) => {
        const playlistId = match?.params.playlistId
        return (
          <AppContainer isPlaylistPlayerPage={!!playlistId}>
            <HeaderWrapper>
              <Header playlistId={playlistId} />
            </HeaderWrapper>
            <VideoFrame
              visible={
                !!lastPlayerState?.currentItem &&
                editMode !== PlaylistEditMode.BULK_EDIT
              }
              isMiniPlayer={lastPlayerState?.currentList?.id !== playlistId}
            />
            <section>
              <Switch>
                <Route
                  path='/playlist/:playlistId'
                  component={PlaylistFolder}
                />
                <Route path='/' component={PlaylistsCatalog}></Route>
              </Switch>
            </section>
          </AppContainer>
        )
      }}
    />
  )
}
