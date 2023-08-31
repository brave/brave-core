// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css'

import { formatTimeInSeconds } from '../utils/timeFormatter'
import {
  DragController,
  InitialState,
  Target
} from '../utils/dragDropController'

interface Props {
  videoElement: HTMLVideoElement | null
}

const SeekerContainer = styled.div`
  display: flex;
  flex-direction: column;
`

const TimeContainer = styled.div`
  display: grid;
  grid-template-columns: auto auto;
  justify-content: space-between;

  color: ${color.text.secondary};
  font: ${font.primary.xSmall.regular};
`

const StyledProgress = styled.progress.attrs(
  (p: { value: number; max: number }) => ({
    'style': {
      '--progress-thumb-left': `calc(${
        (p.value && p.max ? +p.value / +p.max : 0) * 100
      }% + var(--progress-bar-height) * -0.5)`
    }
  })
)`
  --progress-background: #423eee; /* TODO(sko) Neither color.button.background or color.semantic.button.background exist for now */
  --progress-bar-height: 12px;
  --progress-stroke-thickness: 2px;

  width: 100%;
  height: var(--progress-bar-height);
  appearance: none;
  position: relative;

  &::-webkit-progress-bar {
    background: rgba(0, 0, 0, 0.1);
    height: var(--progress-stroke-thickness);
    border-radius: calc(var(--progress-stroke-thickness) * 0.5);
    transform: translate(0, calc((var(--progress-bar-height) - 100%) / 2));
  }
  &::-webkit-progress-value {
    border-radius: calc(var(--progress-stroke-thickness) * 0.5);
    background: var(--progress-background);
  }
  &::after {
    content: '';
    position: absolute;
    top: 0;
    left: var(--progress-thumb-left);
    width: var(--progress-bar-height);
    height: var(--progress-bar-height);
    background: var(--progress-background);
    border-radius: calc(var(--progress-bar-height) * 2);
  }
`

class SeekerDragController extends DragController<
  InitialState & { playing: boolean; value: number },
  Target
> {
  constructor (
    videoElement: HTMLVideoElement,
    progressElem: React.RefObject<HTMLProgressElement>,
    updateProgressValue: (value: number) => void
  ) {
    super('horizontal')

    this.initialState = {
      clientPos: 0,
      target: undefined,
      playing: !videoElement.paused,
      value: progressElem.current!.value
    }

    this.#videoElement = videoElement
    this.#progressElem = progressElem.current!
    this.#updateProgressValue = updateProgressValue

    this.addTargetElement({ elementRef: progressElem })
  }

  #videoElement: HTMLVideoElement
  #progressElem: HTMLProgressElement
  #updateProgressValue: (value: number) => void

  interpolatePxToValue = (clientX: number) => {
    // clientWidth : clientX = max : newValue
    return (clientX / this.#progressElem.clientWidth) * this.#progressElem.max
  }

  onDragStart = () => {
    this.initialState = {
      ...this.initialState,
      playing: !this.#videoElement.paused,
      value: this.#progressElem.value
    }

    // Pause video during drag-and-drop session.
    this.#videoElement.pause()

    this.#videoElement.currentTime = this.interpolatePxToValue(
      this.initialState.clientPos - this.#progressElem.offsetLeft
    )
  }

  onDragUpdate = (target: Target, clientPos: number) => {
    // Update value
    const newTime = this.interpolatePxToValue(
      clientPos - this.#progressElem.offsetLeft
    )

    const oldTime = this.#videoElement.currentTime
    this.#videoElement.currentTime = newTime

    if (oldTime === this.#videoElement.currentTime) {
      // In case the newTime has changed by time smaller than a second,
      // the |currentTime| won't be updated. In this case <progress> could feel
      // like jank. In order to avoid that, force to update the progress ui.
      this.#updateProgressValue(newTime)
    }
  }

  onDragEnd = (target: Target, clientPos: number) => {
    this.#videoElement.currentTime = this.interpolatePxToValue(
      clientPos - this.#progressElem.offsetLeft
    )

    // Restore the playing state.
    if (this.#videoElement.paused && this.initialState.playing) {
      this.#videoElement.play()
    }
  }

  onClick = () => {
    this.#videoElement.currentTime = this.interpolatePxToValue(
      this.initialState.clientPos - this.#progressElem.offsetLeft
    )

    // Restore the playing state.
    if (this.#videoElement.paused && this.initialState.playing) {
      this.#videoElement.play()
    }
  }
}

export default function PlayerSeeker ({ videoElement }: Props) {
  const [duration, setDuration] = React.useState(0)
  const [currentTime, setCurrentTime] = React.useState(0)

  React.useEffect(() => {
    if (!videoElement) {
      // Can be null on start up.
      return
    }

    const onTimeChanged = () => {
      setCurrentTime(videoElement.currentTime)
    }
    const onLoaded = () => {
      setDuration(videoElement.duration)
      onTimeChanged()
    }
    videoElement.addEventListener('loadeddata', onLoaded)
    videoElement.addEventListener('timeupdate', onTimeChanged)
    return () => {
      videoElement.removeEventListener('loadeddata', onLoaded)
      videoElement.removeEventListener('timeupdate', onTimeChanged)
    }
  }, [videoElement])

  const progressElementRef = React.useRef<HTMLProgressElement>(null)
  React.useEffect(() => {
    const progressElem = progressElementRef.current
    if (!progressElem) {
      return
    }

    if (!videoElement) {
      return
    }

    const dragController = new SeekerDragController(
      videoElement,
      progressElementRef,
      setCurrentTime
    )
    return () => {
      dragController.cleanUp()
    }
  }, [progressElementRef.current])

  return (
    <SeekerContainer>
      <StyledProgress
        max={duration}
        value={currentTime}
        ref={progressElementRef}
      />
      <TimeContainer>
        <span>{formatTimeInSeconds(currentTime, 'colon')}</span>
        <span>{formatTimeInSeconds(duration, 'colon')}</span>
      </TimeContainer>
    </SeekerContainer>
  )
}
