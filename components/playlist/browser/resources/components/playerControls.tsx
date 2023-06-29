// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'
import { color } from '@brave/leo/tokens/css'

interface Props {
  videoElementRef: React.RefObject<HTMLVideoElement>
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

export default function PlayerControls ({ videoElementRef, className }: Props) {
  const [isPlaying, setPlaying] = React.useState(false)

  React.useEffect(() => {
    const videoElement = videoElementRef.current
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
  }, [videoElementRef.current])

  return (
    <Container className={className}>
      <div>
        <Control iconName='start-outline' onClick={() => {}}></Control>
        <Control
          iconName='rewind-15'
          onClick={() => {
            if (videoElementRef.current) {
              videoElementRef.current.currentTime -= 15
              if (videoElementRef.current.currentTime < 0)
                videoElementRef.current.currentTime = 0
            }
          }}
        ></Control>
        {isPlaying ? (
          <Control
            iconName='pause-filled'
            onClick={() => {
              videoElementRef.current?.pause()
            }}
          ></Control>
        ) : (
          <Control
            iconName='play-filled'
            onClick={() => {
              videoElementRef.current?.play()
            }}
          ></Control>
        )}
        <Control
          iconName='forward-15'
          onClick={() => {
            if (videoElementRef.current) {
              videoElementRef.current.currentTime += 15
            }
          }}
        ></Control>
        <Control iconName='end-outline' onClick={() => {}}></Control>
      </div>
      <div>
        <Control iconName='autoplay-off' onClick={() => {}}></Control>
        <Control iconName='shuffle-on' onClick={() => {}}></Control>
        <Control iconName='sidepanel-open' onClick={() => {}}></Control>
        <Control iconName='picture-in-picture' onClick={() => {}}></Control>
        <Control iconName='fullscreen-on' onClick={() => {}}></Control>
      </div>
    </Container>
  )
}
