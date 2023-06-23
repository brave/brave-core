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

const HeaderWrapper = styled.header`
  position: sticky;
  width: 100%;
  height: 56px;
  top: 0;
  z-index: 1;
`

export default function App () {
  return (
    <>
      <HeaderWrapper>
        <Route
          path={'/playlist/:playlistId'}
          children={({ match }) => (
            <Header playlistId={match?.params.playlistId} />
          )}
        />
      </HeaderWrapper>
      <section>
        <Switch>
          <Route path='/playlist/:playlistId' component={PlaylistPlayer} />
          <Route path='/' component={PlaylistsCatalog}></Route>
        </Switch>
      </section>
    </>
  )
}
