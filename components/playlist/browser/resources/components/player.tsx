// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

import { Playlist } from 'components/definitions/playlist'
import * as playlistActions from '../actions/playlist_action_creators'
import * as PlaylistMojo from 'gen/brave/components/playlist/mojom/playlist.mojom.m.js'

interface Props {
  actions: any
  currentItem?: PlaylistMojo.PlaylistItem
}

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(playlistActions, dispatch)
})

const mapStateToProps = (state: Playlist.ApplicationState) => ({
  currentItem: state.playerState?.currentItem
})

function Player ({ currentItem }: Props) {
  return (
    <video style={{ width: '512px', height: '288px' }} autoPlay controls id="player" src={currentItem?.mediaPath.url}/>
  )
}

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(Player)
