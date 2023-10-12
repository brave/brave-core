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
import { ApplicationState } from '../reducers/states'
import { hiddenOnMiniPlayer, hiddenOnNormalPlayer } from '../constants/style'

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
  align-items: center;
  display: flex;
`

const NormalPlayerButton = styled(StyledButton)`
  ${hiddenOnMiniPlayer}
`

const MiniPlayerButton = styled(StyledButton)`
  ${hiddenOnNormalPlayer}
`

function Control ({
  iconName,
  size,
  visibility,
  onClick
}: {
  iconName: string
  size: 'jumbo' | 'large'
  visibility: 'mini' | 'normal' | 'both'
  onClick: () => void
}) {
  const Button =
    visibility === 'both'
      ? StyledButton
      : visibility === 'mini'
      ? MiniPlayerButton
      : NormalPlayerButton
  return (
    <Button kind='plain-faint' size={size} onClick={onClick}>
      <Icon name={iconName}></Icon>
    </Button>
  )
}

export default function PlayerControls ({ videoElement, className }: Props) {
  const [isPlaying, setPlaying] = React.useState(false)

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
          iconName='previous-outline'
          size='jumbo'
          visibility='normal'
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
          visibility='normal'
          onClick={() => videoElement && (videoElement.currentTime -= 15)}
        ></Control>
        {isPlaying ? (
          <Control
            iconName='pause-filled'
            size='jumbo'
            visibility='both'
            onClick={() => videoElement?.pause()}
          ></Control>
        ) : (
          <Control
            iconName='play-filled'
            size='jumbo'
            visibility='both'
            onClick={() => videoElement?.play()}
          ></Control>
        )}
        <Control
          iconName='forward-15'
          size='jumbo'
          visibility='normal'
          onClick={() => videoElement && (videoElement.currentTime += 15)}
        ></Control>
        <Control
          iconName='next-outline'
          size='jumbo'
          visibility='normal'
          onClick={() => getPlayerActions().playNextItem()}
        ></Control>
        <Control
          iconName='close'
          size='jumbo'
          visibility='mini'
          onClick={() => getPlayerActions().unloadPlaylist()}
        ></Control>
      </div>
      <div>
        <Control
          iconName={shuffleEnabled ? 'shuffle-on' : 'shuffle-off'}
          size='large'
          visibility='normal'
          onClick={() => getPlayerActions().toggleShuffle()}
        ></Control>
        {/* TODO(sko) We disabled PIP and fullscreen button at the moment */}
      </div>
    </Container>
  )
}
