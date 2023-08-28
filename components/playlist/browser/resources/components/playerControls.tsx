// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import styled from 'styled-components'

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { spacing } from '@brave/leo/tokens/css'

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
    gap: ${spacing.m};
  }
`

const StyledButton = styled(Button)`
  --leo-button-padding: ${spacing.s};
  display: flex;
  align-items: center;
`

function Control ({
  iconName,
  size,
  onClick
}: {
  iconName: string
  size: 'jumbo' | 'large'
  onClick: () => void
}) {
  return (
    <StyledButton kind='plain-faint' size={size} onClick={onClick}>
      <Icon name={iconName}></Icon>
    </StyledButton>
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
          size='jumbo'
          onClick={() => {
            if (!videoElement) return

            if (videoElement.currentTime > 5) {
              videoElement.currentTime = 0
            } else {
              getPlayerActions().playPreviousItem()
            }
          }}
        ></Control>
        <Control
          iconName='rewind-15'
          size='jumbo'
          onClick={() => videoElement && (videoElement.currentTime -= 15)}
        ></Control>
        {isPlaying ? (
          <Control
            iconName='pause-filled'
            size='jumbo'
            onClick={() => videoElement?.pause()}
          ></Control>
        ) : (
          <Control
            iconName='play-filled'
            size='jumbo'
            onClick={() => videoElement?.play()}
          ></Control>
        )}
        <Control
          iconName='forward-15'
          size='jumbo'
          onClick={() => videoElement && (videoElement.currentTime += 15)}
        ></Control>
        <Control
          iconName='end-outline'
          size='jumbo'
          onClick={() => getPlayerActions().playNextItem()}
        ></Control>
      </div>
      <div>
        <Control
          iconName={autoPlayEnabled ? 'autoplay-on' : 'autoplay-off'}
          size='large'
          onClick={() => getPlayerActions().toggleAutoPlay()}
        ></Control>
        <Control
          iconName={shuffleEnabled ? 'shuffle-on' : 'shuffle-off'}
          size='large'
          onClick={() => getPlayerActions().toggleShuffle()}
        ></Control>
        <Control
          iconName='sidepanel-open'
          size='large'
          onClick={() => {}}
        ></Control>
        <Control
          iconName='picture-in-picture'
          size='large'
          onClick={() => videoElement?.requestPictureInPicture()}
        ></Control>
        <Control
          iconName='fullscreen-on'
          size='large'
          onClick={() => videoElement?.requestFullscreen()}
        ></Control>
      </div>
    </Container>
  )
}
