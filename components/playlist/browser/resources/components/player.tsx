// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import styled, { css } from 'styled-components'

import {
  Playlist,
  PlaylistItem
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { color, font, radius, spacing } from '@brave/leo/tokens/css'

import {
  ApplicationState,
  PlayerState,
  useAutoPlayEnabled
} from '../reducers/states'
import { getPlayerActions } from '../api/getPlayerActions'
import PlayerControls from './playerControls'
import PlayerSeeker from './playerSeeker'
import {
  playerTypes,
  playerVariables,
  hiddenOnMiniPlayer,
  hiddenOnNormalPlayer
} from '../constants/style'
import {
  notifyEventsToTopFrame,
  PlaylistTypes
} from '../api/playerEventsNotifier'
import { ProgressBar } from './progressBar'

const VideoContainer = styled.div`
  position: relative;
`

const StyledVideo = styled.video`
  @media ${playerTypes.normalPlayer} {
    aspect-ratio: 16 / 9;
    width: 100vw;
  }
  @media ${playerTypes.miniPlayer} {
    height: 100%;
    width: var(--mini-player-video-width);
    max-width: var(--mini-player-video-width);
    object-fit: cover;
    object-position: center;
  }
`

const StyledSeeker = styled(PlayerSeeker)`
  ${hiddenOnMiniPlayer}
`

const ControlsContainer = styled.div`
  display: flex;
  position: relative;

  @media ${playerTypes.normalPlayer} {
    position: absolute;
    bottom: 0;
    z-index: 1;
    width: 100%;

    flex-direction: column;
    padding: ${spacing.m} ${spacing.xl};
    gap: ${spacing.m};
    flex-shrink: 0;
    justify-content: center;
    box-sizing: border-box;
    height: var(--player-controls-area-height);

    &::before {
      content: '';
      background-color: ${color.dialogs.frostedGlassBackground};
      background-position: center;
      position: absolute;
      width: 100%;
      height: calc(100% + ${spacing.m});
      z-index: -1;
      left: 0;
      bottom: 0;
    }
  }

  @media ${playerTypes.miniPlayer} {
    max-width: calc(100% - var(--mini-player-video-width) - ${spacing.xl});
    flex: 1 1 auto;
    flex-direction: row;
    gap: ${spacing.xl};
    padding-right: ${spacing.xl};
  }
`

const PlayerContainer = styled.div<{ backgroundUrl?: string }>`
  ${playerVariables}
  --mini-player-video-width: 120px;

  width: 100vw;
  height: 100vh;
  position: relative;
  overflow: hidden;
  user-select: none; // In order to drag-and-drop on the seeker works well.

  // Put blurred thumbnail underneath other UI controls
  &::before {
    content: '';
    background: url(${p => p.backgroundUrl});
    filter: blur(8px);
    background-position: center;
    position: absolute;
    height: 100%;
    width: 100%;
    opacity: 0.2;
    z-index: -1;
  }

  @media ${playerTypes.normalPlayer} {
    display: flex;
    flex-direction: column;
  }

  @media ${playerTypes.miniPlayer} {
    display: flex;
    flex-direction: row;
    gap: ${spacing.xl};
  }
`

const FaviconAndTitle = styled.div`
  flex: 1 1 auto;
  display: flex;
  align-items: center;
  gap: ${spacing.m};
  overflow: hidden;
`

const StyledFavicon = styled.img<{ clickable: boolean }>`
  ${hiddenOnMiniPlayer}
  padding: 3px;
  width: 14px;
  height: 14px;
  border-radius: ${radius.s};
  border: 1px solid rgba(0, 0, 0, 0.05);
  background: ${color.white};
  ${p =>
    p.clickable &&
    css`
      cursor: pointer;
    `}
`

const StyledTitle = styled.div<{ clickable: boolean }>`
  color: ${color.text.primary};
  font: ${font.primary.large.semibold};
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  ${p =>
    p.clickable &&
    css`
      cursor: pointer;
    `}
`

const StyledProgressBar = styled(ProgressBar)`
  ${hiddenOnNormalPlayer}
`

// Route changes of PlayerState to parent frame.
function useStateRouter () {
  const playerState = useSelector<ApplicationState, PlayerState | undefined>(
    applicationState => applicationState.playerState
  )
  React.useEffect(() => {
    if (playerState) {
      notifyEventsToTopFrame({
        type: PlaylistTypes.PLAYLIST_PLAYER_STATE_CHANGED,
        data: playerState
      })
    }
  }, [playerState])
}

export default function Player () {
  useStateRouter()

  const currentItem = useSelector<ApplicationState, PlaylistItem | undefined>(
    applicationState => applicationState.playerState?.currentItem
  )
  const currentList = useSelector<ApplicationState, Playlist | undefined>(
    applicationState => applicationState.playerState?.currentList
  )

  const autoPlayEnabled = useAutoPlayEnabled()

  const [videoElement, setVideoElement] =
    React.useState<HTMLVideoElement | null>(null)

  const [currentPosition, setCurrentPosition] = React.useState(0)

  React.useEffect(() => {
    if (videoElement && !videoElement.paused && !currentItem) {
      // This could happen when the current item was deleted. In this case,
      // we should pause the video first. Otherwise, video will keep playing
      // even we clear the src attribute.
      videoElement.pause()
    }

    if (videoElement && currentItem) {
      if (
        videoElement.src !== currentItem?.mediaSource.url &&
        videoElement.src !== currentItem?.mediaPath.url
      ) {
        videoElement.src = currentItem.mediaPath.url
        if (currentItem.lastPlayedPosition < +currentItem.duration / 1e6 - 5) {
          // When the last played position is close to the end(5sec), don't
          // set last position to the current time.
          videoElement.currentTime = currentItem.lastPlayedPosition
        }
      }

      navigator.mediaSession.metadata = new MediaMetadata({
        title: currentItem.name
        // TODO(sko) We might be able to show our thumbnail as artwork, but it seems
        // to need some modification from MediaNotificationView to get image via
        // chrome://playlist-data.
        // Also we might want to add action handlers like "nextTrack"
        // https://developer.mozilla.org/en-US/docs/Web/API/MediaSession
      })
    } else {
      // In order to reset media control UI on the toolbar, we need to clear
      // the src attribute. Otherwise, media control UI will remain visible
      // even though we clear the metadata.
      if (videoElement) videoElement.src = ''
      navigator.mediaSession.metadata = null
    }
  }, [currentItem, videoElement])

  return (
    <PlayerContainer backgroundUrl={currentItem?.thumbnailPath.url}>
      <VideoContainer>
        <StyledVideo
          tabIndex={0}
          onKeyDown={e => e.key === 'Enter' && e.currentTarget.click()}
          ref={setVideoElement}
          autoPlay
          onPlay={() =>
            getPlayerActions().playerStartedPlayingItem(currentItem)
          }
          onPause={() =>
            getPlayerActions().playerStoppedPlayingItem(currentItem)
          }
          onEnded={() => {
            if (currentItem) {
              setCurrentPosition(videoElement!.duration)

              notifyEventsToTopFrame({
                type: PlaylistTypes.PLAYLIST_LAST_PLAYED_POSITION_OF_CURRENT_ITEM_CHANGED,
                data: {
                  ...currentItem,
                  lastPlayedPosition: videoElement!.duration
                }
              })
            }

            getPlayerActions().playerStoppedPlayingItem(currentItem)
            if (autoPlayEnabled) getPlayerActions().playNextItem() // In case the current item is the last one, nothing will happen
          }}
          onTimeUpdate={() => {
            if (!currentItem) return

            setCurrentPosition(videoElement!.currentTime)
            notifyEventsToTopFrame({
              type: PlaylistTypes.PLAYLIST_LAST_PLAYED_POSITION_OF_CURRENT_ITEM_CHANGED,
              data: {
                ...currentItem,
                lastPlayedPosition: videoElement!.currentTime
              }
            })
          }}
          onError={() => {
            // TODO(sko) Show alert when z-index patch is ready.
            if (autoPlayEnabled) getPlayerActions().playNextItem() // In case the current item is the last one, nothing will happen
          }}
        />
        {!!currentItem?.duration && (
          <StyledProgressBar
            progress={currentPosition / (+currentItem.duration / 1e6)}
          />
        )}
      </VideoContainer>
      <ControlsContainer>
        <FaviconAndTitle>
          <StyledFavicon
            src={
              currentItem?.id &&
              `chrome-untrusted://playlist-data/${currentItem.id}/favicon`
            }
            clickable={!!currentItem}
            tabIndex={0}
            onKeyDown={e => e.key === 'Enter' && e.currentTarget.click()}
            onClick={() => {
              if (currentItem) {
                notifyEventsToTopFrame({
                  type: PlaylistTypes.PLAYLIST_OPEN_SOURCE_PAGE,
                  data: currentItem
                })
              }
            }}
          />
          <StyledTitle
            clickable={!!(currentList && currentItem)}
            onClick={() => {
              if (currentList && currentItem) {
                notifyEventsToTopFrame({
                  type: PlaylistTypes.PLAYLIST_GO_BACK_TO_CURRENTLY_PLAYING_FOLDER,
                  data: { currentList, currentItem }
                })
              }
            }}
            tabIndex={0}
            onKeyDown={e => e.key === 'Enter' && e.currentTarget.click()}
          >
            {currentItem?.name}
          </StyledTitle>
        </FaviconAndTitle>
        <StyledSeeker videoElement={videoElement} />
        <PlayerControls videoElement={videoElement} />
      </ControlsContainer>
    </PlayerContainer>
  )
}
