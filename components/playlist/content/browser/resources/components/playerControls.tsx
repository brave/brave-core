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
    align-items: center;
    gap: ${spacing.s};
  }
`

const StyledButton = styled(Button)`
  color: var(--leo-color-icon-default);
  --leo-button-padding: ${spacing.s};
  align-items: center;
  display: flex;
  width: var(--leo-icon-xl);
  height: var(--leo-icon-xl);
`

const NormalPlayerButton = styled(StyledButton)`
  ${hiddenOnMiniPlayer}
`

const MiniPlayerButton = styled(StyledButton)`
  ${hiddenOnNormalPlayer}
`

function Control ({
  iconName,
  visibility,
  title,
  kind,
  onClick
}: {
  iconName: string
  title: string
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
      size='jumbo'
      onClick={onClick}
      title={title}
      fab
    >
      <Icon name={iconName}></Icon>
    </Button>
  )
}

export default function PlayerControls ({ videoElement, className }: Props) {
  const [isPlaying, setPlaying] = React.useState(false)
  const [isMuted, setIsMuted] = React.useState(false)

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
      const updateMuted = () => setIsMuted(videoElement.muted)

      videoElement.addEventListener('click', togglePlayingState)
      videoElement.addEventListener('playing', notifyPlaying)
      videoElement.addEventListener('pause', notifyPaused)
      videoElement.addEventListener('ended', notifyPaused)
      videoElement.addEventListener('volumechange', updateMuted)
      return () => {
        videoElement.removeEventListener('click', togglePlayingState)
        videoElement.removeEventListener('playing', notifyPlaying)
        videoElement.removeEventListener('pause', notifyPaused)
        videoElement.removeEventListener('ended', notifyPaused)
        videoElement.removeEventListener('volumechange', updateMuted)
      }
    }

    return () => {}
  }, [videoElement])

  return (
    <Container className={className}>
      <div>
        <Control
          iconName='previous-outline'
          visibility='normal'
          title={getLocalizedString('bravePlaylistTooltipPrevious')}
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
          visibility='normal'
          title={getLocalizedString('bravePlaylistTooltipRewind')}
          kind='plain-faint'
          onClick={() => videoElement && (videoElement.currentTime -= 15)}
        />
        {isPlaying ? (
          <Control
            iconName='pause-filled'
            visibility='both'
            title={getLocalizedString('bravePlaylistTooltipPause')}
            kind='plain-faint'
            onClick={() => videoElement?.pause()}
          />
        ) : (
          <Control
            iconName='play-filled'
            visibility='both'
            title={getLocalizedString('bravePlaylistTooltipPlay')}
            kind='plain-faint'
            onClick={() => videoElement?.play()}
          />
        )}
        <Control
          iconName='forward-15'
          visibility='normal'
          title={getLocalizedString('bravePlaylistTooltipForward')}
          kind='plain-faint'
          onClick={() => videoElement && (videoElement.currentTime += 15)}
        />
        <Control
          iconName='next-outline'
          visibility='normal'
          title={getLocalizedString('bravePlaylistTooltipNext')}
          kind='plain-faint'
          onClick={() => getPlayerActions().playNextItem()}
        />
        <Control
          iconName='close'
          visibility='mini'
          title={getLocalizedString('bravePlaylistTooltipClose')}
          kind='plain-faint'
          onClick={() => getPlayerActions().unloadPlaylist()}
        />
      </div>
      <div>
        <Control
          iconName={isMuted ? 'volume-off' : 'volume-on'}
          visibility='normal'
          title={getLocalizedString('bravePlaylistTooltipToggleMuted')}
          kind={'plain-faint'}
          onClick={() => {
            if (videoElement) videoElement.muted = !isMuted
          }}
        />
        <Control
          iconName={shuffleEnabled ? 'shuffle-toggle-on' : 'shuffle-off'}
          visibility='normal'
          title={getLocalizedString('bravePlaylistTooltipShuffle')}
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
          visibility='normal'
          title={getLocalizedString(
            !loopMode
              ? 'bravePlaylistTooltipLoopOff'
              : loopMode === 'single-item'
              ? 'bravePlaylistTooltipLoopOne'
              : 'bravePlaylistTooltipLoopAll'
          )}
          kind={loopMode ? 'plain' : 'plain-faint'}
          onClick={() => getPlayerActions().advanceLoopMode()}
        />
        {/* TODO(sko) We disabled PIP and fullscreen button at the moment */}
      </div>
    </Container>
  )
}
