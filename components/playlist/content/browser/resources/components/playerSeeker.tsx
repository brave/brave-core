// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import styled, { css } from 'styled-components'
import { color } from '@brave/leo/tokens/css/variables'

interface Props {
  videoElement: HTMLVideoElement | null
  className?: string
  thumbVisible: boolean
  onMouseEnter: () => void
  onMouseLeave: () => void
}

const SeekerContainer = styled.div`
  display: flex;
  flex-direction: column;
`

const StyledProgress = styled.progress.attrs(
  (p: { value: number; max: number; thumbVisible: boolean }) => ({
    'style': {
      '--progress-thumb-left': `calc(${
        (p.value && p.max ? +p.value / +p.max : 0) * 100
      }% + var(--progress-bar-height) * -0.5)`
    }
  })
)`
  --progress-background: var(--seeker-progress-background, rgba(0, 0, 0, 0.1));
  --progress-stroke-background: ${color.button.background};
  --progress-bar-height: var(--seeker-height);
  --progress-stroke-thickness: var(--seeker-progress-stroke-thickness);

  width: 100%;
  height: var(--progress-bar-height);
  appearance: none;
  position: relative;

  &::-webkit-progress-bar {
    background: var(--progress-background);
    height: var(--progress-stroke-thickness);
    border-radius: calc(var(--progress-stroke-thickness) * 0.5);
    transform: translate(0, calc((var(--progress-bar-height) - 100%) / 2));
  }
  &::-webkit-progress-value {
    border-radius: calc(var(--progress-stroke-thickness) * 0.5);
    background: var(--progress-stroke-background);
  }

  &::after {
    content: '';
    position: absolute;
    top: 0;
    left: var(--progress-thumb-left);
    width: var(--progress-bar-height);
    height: var(--progress-bar-height);
    background: var(--progress-stroke-background);
    border-radius: calc(var(--progress-bar-height) * 2);

    transition: transform 0.1s ease-out;
    ${(p) =>
      !p.thumbVisible &&
      css`
        transform: scale(0);
      `}
  }
`

class DragController {
  constructor(
    videoElement: HTMLVideoElement,
    progressElem: HTMLProgressElement,
    updateProgressValue: (value: number) => void,
    onDragStateChanged: (dragging: boolean) => void
  ) {
    this.#videoElement = videoElement
    this.#progressElem = progressElem
    this.#updateProgressValue = updateProgressValue
    this.#onDragStateChanged = onDragStateChanged
  }

  #videoElement: HTMLVideoElement
  #progressElem: HTMLProgressElement
  #updateProgressValue: (value: number) => void
  #onDragStateChanged: (dragging: boolean) => void

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
    if (!this.#videoElement) {
      return
    }

    this.#isMouseDown = true
    this.#initialState = {
      playing: !this.#videoElement?.paused,
      clientX: event.clientX,
      value: this.#progressElem.value
    }

    // Pause video during drag-and-drop session.
    this.#videoElement.pause()

    this.#videoElement.currentTime = this.interpolatePxToValue(
      this.#initialState.clientX - this.#progressElem.offsetLeft
    )

    document.addEventListener('mousemove', this.onMouseMove)
    document.addEventListener('mouseup', this.onMouseUp)
  }

  onMouseMove = (event: MouseEvent) => {
    if (!this.#videoElement) {
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
      this.#onDragStateChanged(this.#isDragging)
    }

    if (!this.#isDragging) {
      return
    }

    // Update value
    const newTime = this.interpolatePxToValue(
      event.clientX - this.#progressElem.offsetLeft
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

  onMouseUp = (event: MouseEvent) => {
    document.removeEventListener('mousemove', this.onMouseMove)
    document.removeEventListener('mouseup', this.onMouseUp)

    if (!this.#videoElement) {
      return
    }

    this.#isMouseDown = false

    if (this.#isDragging) {
      this.#videoElement.currentTime = this.interpolatePxToValue(
        event.clientX - this.#progressElem.offsetLeft
      )
      this.#isDragging = false
      this.#onDragStateChanged(this.#isDragging)
    } else {
      this.#videoElement.currentTime = this.interpolatePxToValue(
        this.#initialState!.clientX - this.#progressElem.offsetLeft
      )
    }

    // Restore the playing state.
    if (this.#videoElement.paused && this.#initialState!.playing) {
      this.#videoElement.play()
    }
  }
}

export default function PlayerSeeker({
  videoElement,
  className,
  thumbVisible,
  onMouseEnter,
  onMouseLeave
}: Props) {
  const [duration, setDuration] = React.useState(0)
  const [currentTime, setCurrentTime] = React.useState(0)
  const [dragging, setDragging] = React.useState(false)

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

    const dragController = new DragController(
      videoElement,
      progressElem,
      setCurrentTime,
      setDragging
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
    <SeekerContainer
      className={className}
      onMouseEnter={() => onMouseEnter()}
      onMouseLeave={() => onMouseLeave()}
    >
      <StyledProgress
        max={duration}
        value={currentTime}
        ref={progressElementRef}
        thumbVisible={thumbVisible || dragging}
      />
    </SeekerContainer>
  )
}
