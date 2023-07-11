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
import PlaylistPlayer from './playlistPlayer'

const HeaderWrapper = styled.header<{ isPlaylistPlayerPage: boolean }>`
  position: sticky;
  width: 100%;
  height: ${({ isPlaylistPlayerPage }) =>
    isPlaylistPlayerPage ? '74px' : '56px'};
  top: 0;
  z-index: 1;
`

export default function App () {
  return (
    <>
      <Route
        path={'/playlist/:playlistId'}
        children={({ match }) => {
          const playlistId = match?.params.playlistId
          return (
            <HeaderWrapper isPlaylistPlayerPage={!!playlistId}>
              <Header playlistId={playlistId} />
            </HeaderWrapper>
          )
        }}
      />
      <section>
        <Switch>
          <Route path='/playlist/:playlistId' component={PlaylistPlayer} />
          <Route path='/' component={PlaylistsCatalog}></Route>
        </Switch>
      </section>
    </>
  )
}
