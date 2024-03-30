/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Route, Switch } from 'react-router-dom'
import styled from 'styled-components'

import AlertCenter from '@brave/leo/react/alertCenter'

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
import { HistoryContext } from './historyContext'

const AppContainer = styled.div<{ isPlaylistPlayerPage: boolean }>`
  --header-height: ${({ isPlaylistPlayerPage }) =>
    isPlaylistPlayerPage ? '74px' : '56px'};
`

const StickyArea = styled.header`
  position: sticky;
  width: 100vw;
  top: 0;
  z-index: 1;
`

const StyledHeader = styled(Header)`
  height: var(--header-height);
`

export default function App () {
  const lastPlayerState = useLastPlayerState()
  const editMode = usePlaylistEditMode()

  return (
    <HistoryContext>
      <Route
        path={'/playlist/:playlistId'}
        children={({ match }) => {
          const playlistId = match?.params.playlistId
          return (
            <AppContainer isPlaylistPlayerPage={!!playlistId}>
              <StickyArea>
                <StyledHeader playlistId={playlistId} />
                <AlertCenter />
                <VideoFrame
                  visible={
                    !!lastPlayerState?.currentItem &&
                    editMode !== PlaylistEditMode.BULK_EDIT
                  }
                  isMiniPlayer={lastPlayerState?.currentList?.id !== playlistId}
                />
              </StickyArea>
              <section>
                <Switch>
                  <Route
                    path='/playlist/:playlistId'
                    component={PlaylistFolder}
                  />
                  <Route
                    path='/'
                    component={PlaylistsCatalog}
                  ></Route>
                </Switch>
              </section>
            </AppContainer>
          )
        }}
      />
    </HistoryContext>
  )
}
