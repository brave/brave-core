// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'
import { color } from '@brave/leo/tokens/css'

import { getPlayerActions } from '../api/getPlayerActions'
import { ApplicationState, useAutoPlayEnabled } from '../reducers/states'

interface Props {
  videoElement?: HTMLVideoElement | null
  className?: string
}

const Container = styled.div`
  display: grid;
  grid-template-columns: auto auto;
  justify-content: space-between;

  & > div {
    display: flex;
    gap: 16px;
  }
`

const StyledIcon = styled(Icon)`
  color: ${color.icon.default};
  cursor: pointer;
`

function Control ({
  iconName,
  onClick
}: {
  iconName: string
  onClick: () => void
}) {
  return (
    <div onClick={onClick}>
      <StyledIcon name={iconName}></StyledIcon>
    </div>
  )
}

export default function PlayerControls ({ videoElement, className }: Props) {
  const [isPlaying, setPlaying] = React.useState(false)

  const autoPlayEnabled = useAutoPlayEnabled()

  const shuffleEnabled = useSelector<ApplicationState, boolean | undefined>(
    applicationState => applicationState.playerState?.shuffleEnabled
  )

  React.useEffect(() => {
    if (videoElement) {
      const togglePlayingState = () => {
        videoElement.paused ? videoElement.play() : videoElement.pause()
      }
      const notifyPlaying = () => setPlaying(true)
      const notifyPaused = () => setPlaying(false)

      videoElement.addEventListener('click', togglePlayingState)
      videoElement.addEventListener('playing', notifyPlaying)
      videoElement.addEventListener('pause', notifyPaused)
      videoElement.addEventListener('ended', notifyPaused)
      return () => {
        videoElement.removeEventListener('click', togglePlayingState)
        videoElement.removeEventListener('playing', notifyPlaying)
        videoElement.removeEventListener('pause', notifyPaused)
        videoElement.removeEventListener('ended', notifyPaused)
      }
    }

    return () => {}
  }, [videoElement])

  return (
    <Container className={className}>
      <div>
        <Control
          iconName='start-outline'
          onClick={() => getPlayerActions().playPreviousItem()}
        ></Control>
        <Control
          iconName='rewind-15'
          onClick={() => videoElement && (videoElement.currentTime -= 15)}
        ></Control>
        {isPlaying ? (
          <Control
            iconName='pause-filled'
            onClick={() => videoElement?.pause()}
          ></Control>
        ) : (
          <Control
            iconName='play-filled'
            onClick={() => videoElement?.play()}
          ></Control>
        )}
        <Control
          iconName='forward-15'
          onClick={() => videoElement && (videoElement.currentTime += 15)}
        ></Control>
        <Control
          iconName='end-outline'
          onClick={() => getPlayerActions().playNextItem()}
        ></Control>
      </div>
      <div>
        <Control
          iconName={autoPlayEnabled ? 'autoplay-on' : 'autoplay-off'}
          onClick={() => getPlayerActions().toggleAutoPlay()}
        ></Control>
        <Control
          iconName={shuffleEnabled ? 'shuffle-on' : 'shuffle-off'}
          onClick={() => getPlayerActions().toggleShuffle()}
        ></Control>
        <Control iconName='sidepanel-open' onClick={() => {}}></Control>
        <Control
          iconName='picture-in-picture'
          onClick={() => videoElement?.requestPictureInPicture()}
        ></Control>
        <Control
          iconName='fullscreen-on'
          onClick={() => videoElement?.requestFullscreen()}
        ></Control>
      </div>
    </Container>
  )
}
