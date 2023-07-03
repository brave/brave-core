// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css'

interface Props {
  videoElementRef: React.RefObject<HTMLVideoElement>
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

const StyledProgress = styled.progress`
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
    opacity: 1;
    transform: translate(0, calc((var(--progress-bar-height) - 100%) / 2));
  }
  &::-webkit-progress-value {
    border-radius: calc(var(--progress-stroke-thickness) * 0.5);
    background: var(--progress-background);
  }
  // TODO(sko) Can we split this so that we can avoid creating new class too
  // many times?
  &::after {
    content: '';
    position: absolute;
    top: 0;
    left: calc(
      ${p => (p.value && p.max ? +p.value / +p.max : 0) * 100}% +
        var(--progress-bar-height) * -0.5
    );
    width: var(--progress-bar-height);
    height: var(--progress-bar-height);
    background: var(--progress-background);
    border-radius: calc(var(--progress-bar-height) * 2);
  }
`

function formatTime (time: number) {
  const stringFormat = (t: number) => String(t).padStart(2, '0')
  const hours = stringFormat(Math.floor(time / 3600))
  const minutes = stringFormat(Math.floor((time % 3600) / 60))
  const seconds = stringFormat(Math.floor(time % 60))
  return `${hours}:${minutes}:${seconds}`
}

class DragController {
  constructor (
    videoRef: React.RefObject<HTMLVideoElement>,
    progressElem: HTMLProgressElement,
    updateProgressValue: (value: number) => void
  ) {
    this.#videoRef = videoRef
    this.#progressElem = progressElem
    this.#updateProgressValue = updateProgressValue
  }

  #videoRef: React.RefObject<HTMLVideoElement>
  #progressElem: HTMLProgressElement
  #updateProgressValue: (value: number) => void

  #isMouseDown = false
  #isDragging = false

  // Indicate the state of the video when mouse was down.
  #initialState?: {
    playing: boolean
    clientX: number
    value: number
  } = undefined

  interpolatePxToValue = (clientX: number) => {
    // clientWidth : clientX = max : newValue
    return (clientX / this.#progressElem.clientWidth) * this.#progressElem.max
  }

  onMouseDown = (event: MouseEvent) => {
    if (!this.#videoRef.current) {
      return
    }

    this.#isMouseDown = true
    this.#initialState = {
      playing: !this.#videoRef.current?.paused,
      clientX: event.clientX,
      value: this.#progressElem.value
    }

    // Pause video during drag-and-drop session.
    this.#videoRef.current.pause()

    this.#videoRef.current.currentTime = this.interpolatePxToValue(
      this.#initialState.clientX - this.#progressElem.offsetLeft
    )

    document.addEventListener('mousemove', this.onMouseMove)
    document.addEventListener('mouseup', this.onMouseUp)
  }

  onMouseMove = (event: MouseEvent) => {
    if (!this.#videoRef.current) {
      return
    }

    if (!this.#isMouseDown) {
      return
    }

    const dragThreshold = 5
    if (
      !this.#isDragging &&
      Math.abs(event.clientX - this.#initialState!.clientX) > dragThreshold
    ) {
      this.#isDragging = true
    }

    if (!this.#isDragging) {
      return
    }

    // Update value
    const newTime = this.interpolatePxToValue(
      event.clientX - this.#progressElem.offsetLeft
    )

    const oldTime = this.#videoRef.current.currentTime
    this.#videoRef.current.currentTime = newTime

    if (oldTime === this.#videoRef.current.currentTime) {
      // In case the newTime has changed by time smaller than a second,
      // the |currentTime| won't be updated. In this case <progress> could feel
      // like jank. In order to avoid that, force to update the progress ui.
      this.#updateProgressValue(newTime)
    }
  }

  onMouseUp = (event: MouseEvent) => {
    document.removeEventListener('mousemove', this.onMouseMove)
    document.removeEventListener('mouseup', this.onMouseUp)

    if (!this.#videoRef.current) {
      return
    }

    this.#isMouseDown = false

    if (this.#isDragging) {
      this.#videoRef.current.currentTime = this.interpolatePxToValue(
        event.clientX - this.#progressElem.offsetLeft
      )
    } else {
      this.#videoRef.current.currentTime = this.interpolatePxToValue(
        this.#initialState!.clientX - this.#progressElem.offsetLeft
      )
    }

    // Restore the playing state.
    if (this.#videoRef.current.paused && this.#initialState!.playing) {
      this.#videoRef.current.play()
    }
  }
}

export default function PlayerSeeker ({ videoElementRef }: Props) {
  const [duration, setDuration] = React.useState(0)
  const [currentTime, setCurrentTime] = React.useState(0)

  React.useEffect(() => {
    const videoElem = videoElementRef.current
    if (!videoElem) {
      console.error(`Video element doesn't exist`)
      return
    }

    const onTimeChanged = () => {
      setCurrentTime(videoElem.currentTime)
    }
    const onLoaded = () => {
      setDuration(videoElem.duration)
      onTimeChanged()
    }
    videoElem.addEventListener('loadeddata', onLoaded)
    videoElem.addEventListener('timeupdate', onTimeChanged)
    return () => {
      videoElem.removeEventListener('loadeddata', onLoaded)
      videoElem.removeEventListener('timeupdate', onTimeChanged)
    }
  }, [videoElementRef.current])

  const progressElementRef = React.useRef<HTMLProgressElement>(null)
  React.useEffect(() => {
    const progressElem = progressElementRef.current
    if (!progressElem) {
      console.error(`Progress element doesn't exist`)
      return
    }

    const dragController = new DragController(
      videoElementRef,
      progressElem,
      setCurrentTime
    )
    progressElem.addEventListener('mousedown', dragController.onMouseDown)
    return () => {
      progressElem.removeEventListener('mousedown', dragController.onMouseDown)
      // Make sure we don't have event listeners on document.
      document.removeEventListener('mousemove', dragController.onMouseMove)
      document.removeEventListener('mousemove', dragController.onMouseUp)
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
        <span>{formatTime(currentTime)}</span>
        <span>{formatTime(duration)}</span>
      </TimeContainer>
    </SeekerContainer>
  )
}
