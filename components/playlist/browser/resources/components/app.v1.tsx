/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Route, Switch } from 'react-router-dom'
import styled, { css } from 'styled-components'

import AlertCenter from '@brave/leo/react/alertCenter'

// Components
import Header from './header'
import PlaylistsCatalog from './playlistsCatalog'
import PlaylistFolder from './playlistFolder'
import VideoFrame from './videoFrame'
import {
  PlaylistEditMode,
  useLastPlayerState,
  usePlaylistEditMode,
  useShouldShowAddMediaFromPage
} from '../reducers/states'
import { HistoryContext } from './historyContext'
import { AddMediaFromPageButton } from './addMediaFromPageButton'
import { color, spacing } from '@brave/leo/tokens/css/variables'

const AppContainer = styled.div<{ isPlaylistPlayerPage: boolean }>`
  --header-height: ${({ isPlaylistPlayerPage }) =>
    isPlaylistPlayerPage ? '74px' : '56px'};
`

const StickyArea = styled.header<{ position: 'top' | 'bottom' }>`
  width: 100vw;
  ${({ position }) =>
    position === 'top'
      ? css`
          top: 0;
          position: sticky;
        `
      : css`
          bottom: 0;
          position: fixed;
        `}
  z-index: 1;
`

const StyledHeader = styled(Header)`
  height: var(--header-height);
`

const Footer = styled.footer`
  padding: ${spacing.xl};
  border-top: 1px solid ${color.divider.subtle};
`

export default function App () {
  const lastPlayerState = useLastPlayerState()
  const editMode = usePlaylistEditMode()
  const shouldShowAddMediaFromPage = useShouldShowAddMediaFromPage()

  return (
    <HistoryContext>
      <Route
        path={'/playlist/:playlistId'}
        children={({ match }) => {
          const playlistId = match?.params.playlistId
          return (
            <AppContainer isPlaylistPlayerPage={!!playlistId}>
              <StickyArea position='top'>
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
              {shouldShowAddMediaFromPage ? (
                <StickyArea position='bottom'>
                  <Footer>
                    <AddMediaFromPageButton />
                  </Footer>
                </StickyArea>
              ) : null}
            </AppContainer>
          )
        }}
      />
    </HistoryContext>
  )
}
