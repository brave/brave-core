// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import styled from 'styled-components'

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { spacing } from '@brave/leo/tokens/css/variables'

import { getPlayerActions } from '../api/getPlayerActions'
import { ApplicationState, useLoopMode } from '../reducers/states'
import { hiddenOnMiniPlayer, hiddenOnNormalPlayer } from '../constants/style'
import { getLocalizedString } from '../utils/l10n'

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
  color: var(--leo-color-icon-default);
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

function Control({
  iconName,
  size,
  visibility,
  title,
  kind,
  onClick
}: {
  iconName: string
  title: string
  size: 'jumbo' | 'large'
  visibility: 'mini' | 'normal' | 'both'
  kind: 'plain' | 'plain-faint'
  onClick: () => void
}) {
  const Button =
    visibility === 'both'
      ? StyledButton
      : visibility === 'mini'
      ? MiniPlayerButton
      : NormalPlayerButton
  return (
    <Button
      kind={kind}
      size={size}
      onClick={onClick}
      title={title}
    >
      <Icon name={iconName}></Icon>
    </Button>
  )
}

export default function PlayerControls({ videoElement, className }: Props) {
  const [isPlaying, setPlaying] = React.useState(false)

  const shuffleEnabled = useSelector<ApplicationState, boolean | undefined>(
    (applicationState) => applicationState.playerState?.shuffleEnabled
  )

  const loopMode = useLoopMode()

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
          title={getLocalizedString('bravePlaylistA11YPrevious')}
          kind='plain-faint'
          onClick={() => {
            if (!videoElement) return

            if (videoElement.currentTime > 5) {
              videoElement.currentTime = 0
            } else {
              getPlayerActions().playPreviousItem()
            }
          }}
        />
        <Control
          iconName='rewind-15'
          size='jumbo'
          visibility='normal'
          title={getLocalizedString('bravePlaylistA11YRewind')}
          kind='plain-faint'
          onClick={() => videoElement && (videoElement.currentTime -= 15)}
        />
        {isPlaying ? (
          <Control
            iconName='pause-filled'
            size='jumbo'
            visibility='both'
            title={getLocalizedString('bravePlaylistA11YPause')}
            kind='plain-faint'
            onClick={() => videoElement?.pause()}
          />
        ) : (
          <Control
            iconName='play-filled'
            size='jumbo'
            visibility='both'
            title={getLocalizedString('bravePlaylistA11YPlay')}
            kind='plain-faint'
            onClick={() => videoElement?.play()}
          />
        )}
        <Control
          iconName='forward-15'
          size='jumbo'
          visibility='normal'
          title={getLocalizedString('bravePlaylistA11YForward')}
          kind='plain-faint'
          onClick={() => videoElement && (videoElement.currentTime += 15)}
        />
        <Control
          iconName='next-outline'
          size='jumbo'
          visibility='normal'
          title={getLocalizedString('bravePlaylistA11YNext')}
          kind='plain-faint'
          onClick={() => getPlayerActions().playNextItem()}
        />
        <Control
          iconName='close'
          size='jumbo'
          visibility='mini'
          title={getLocalizedString('bravePlaylistA11YClose')}
          kind='plain-faint'
          onClick={() => getPlayerActions().unloadPlaylist()}
        />
      </div>
      <div>
        <Control
          iconName={shuffleEnabled ? 'shuffle-toggle-on' : 'shuffle-off'}
          size='large'
          visibility='normal'
          title={getLocalizedString('bravePlaylistA11YShuffle')}
          kind={shuffleEnabled ? 'plain' : 'plain-faint'}
          onClick={() => getPlayerActions().toggleShuffle()}
        />
        <Control
          iconName={
            !loopMode
              ? 'loop-all'
              : loopMode === 'single-item'
              ? 'loop-1-toggle-on'
              : 'loop-all-toggle-on'
          }
          size='large'
          visibility='normal'
          title={getLocalizedString(
            !loopMode
              ? 'bravePlaylistA11YLoopOff'
              : loopMode === 'single-item'
              ? 'bravePlaylistA11YLoopOne'
              : 'bravePlaylistA11YLoopAll'
          )}
          kind={loopMode ? 'plain' : 'plain-faint'}
          onClick={() => getPlayerActions().advanceLoopMode()}
        />
        {/* TODO(sko) We disabled PIP and fullscreen button at the moment */}
      </div>
    </Container>
  )
}
