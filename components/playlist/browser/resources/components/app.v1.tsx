/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { Route, Switch } from 'react-router-dom'
import styled from 'styled-components'

// Types
import { Playlist } from '../../../../definitions/playlist'
import * as playlistActions from '../actions/playlist_action_creators'

// Components
import Header from './header'
import PlaylistsCatalog from './playlistsCatalog'
import VideoFrame from './videoFrame'
import PlaylistItem from './playlistItem'

interface Props {
  actions: typeof playlistActions
  playlistData: Playlist.State
}

const HeaderWrapper = styled.header`
  position: sticky;
  width: 100%;
  height: 56px;
  top: 0;
  z-index: 1;
`

function App (props: Props) {
  return (
    <>
      <HeaderWrapper>
        <Route
          path={'/playlist/:playlistId'}
          children={({ match }) => (
            <Header
              playlist={props.playlistData.lists.find(
                playlist => playlist.id === match?.params.playlistId
              )}
            />
          )}
        />
      </HeaderWrapper>
      <section>
        <Switch>
          <Route
            path='/playlist/:playlistId'
            render={({ match }) => {
              const playlist = props.playlistData.lists.find(
                playlist => playlist.id === match.params.playlistId
              )
              return (
                <>
                  <VideoFrame
                    playing={!!props.playlistData.lastPlayerState?.playing}
                  />
                  {playlist?.items.map(item => (
                    <PlaylistItem
                      id={item.id}
                      name={item.name}
                      thumbnailUrl={item.thumbnailPath.url}
                      onClick={() => {}}
                    />
                  ))}
                </>
              )
            }}
          ></Route>
          <Route path='/'>
            <PlaylistsCatalog playlists={props.playlistData.lists} />
          </Route>
        </Switch>
      </section>
    </>
  )
}

export const mapStateToProps = (state: Playlist.ApplicationState) => ({
  playlistData: state.playlistData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(playlistActions, dispatch)
})

export default connect(mapStateToProps, mapDispatchToProps)(App)
