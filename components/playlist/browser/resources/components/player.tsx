// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import styled from 'styled-components'

import { ApplicationState } from 'components/playlist/browser/resources/reducers/states'
import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { getAllActions } from '../api/getAllActions'

const StyledVideo = styled.video`
  width: 100vw;
  height: 100vh;
`

export default function Player () {
  const currentItem = useSelector<ApplicationState, PlaylistItem | undefined>(
    applicationState => applicationState.playerState?.currentItem
  )

  const playing = useSelector<ApplicationState, PlaylistItem | undefined>(
    applicationState => applicationState.playerState?.currentItem
  )

  // Route changes in props to parent frame.
  React.useEffect(() => {
    window.parent.postMessage(
      {
        currentItem,
        playing
      },
      'chrome-untrusted://playlist'
    )
  })

  return (
    <StyledVideo
      id='player'
      autoPlay
      controls
      onPlay={() => getAllActions().playerStartedPlayingItem(currentItem)}
      onPause={() => getAllActions().playerStoppedPlayingItem(currentItem)}
      onEnded={() => getAllActions().playerStoppedPlayingItem(currentItem)}
      src={currentItem?.mediaPath.url}
    />
  )
}

// export default connect(mapStateToProps, mapDispatchToProps)(Player)
