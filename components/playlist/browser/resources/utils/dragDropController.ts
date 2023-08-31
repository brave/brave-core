// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RefObject } from 'react'

export type Target = {
  elementRef: RefObject<HTMLElement>
}

export type InitialState = {
  clientPos: number
  target: Target | undefined
}

type Orientation = 'horizontal' | 'vertical'

export class DragController<
  InitialStateType extends InitialState,
  TargetType extends Target
> {
  constructor (orientation: Orientation) {
    this.#orientation = orientation
  }

  cleanUp = () => {
    // Make sure we don't have event listeners on document.
    for (const {
      elementRef: { current }
    } of this.targets) {
      current?.removeEventListener('mousedown', this.onMouseDown)
    }
    document.removeEventListener('mousemove', this.onMouseMove)
    document.removeEventListener('mousemove', this.onMouseUp)
  }

  addTargetElement (target: TargetType) {
    if (!target.elementRef.current) {
      throw new Error('call this only when the element is valid')
    }

    target.elementRef.current.addEventListener('mousedown', this.onMouseDown)
    this.targets.push(target)
  }

  initialState: InitialStateType
  canDrag = true
  isDragging = false
  targets: TargetType[] = []

  #orientation: Orientation
  #isMouseDown = false

  onDragStart: (target: TargetType, clientPos: number) => void
  onDragUpdate: (target: TargetType, clientPos: number) => void
  onDragEnd: (target: TargetType, clientPos: number) => void
  onClick: (target: TargetType) => void

  onMouseDown = (event: MouseEvent) => {
    this.#isMouseDown = true
    this.isDragging = false
    this.initialState.clientPos =
      this.#orientation === 'horizontal' ? event.clientX : event.clientY
    this.initialState.target = this.targets.find(
      t => t.elementRef.current === event.currentTarget
    )!

    document.addEventListener('mousemove', this.onMouseMove)
    document.addEventListener('mouseup', this.onMouseUp)

    if (this.canDrag) {
      this.onDragStart(
        this.initialState.target as TargetType,
        this.#orientation === 'horizontal' ? event.clientX : event.clientY
      )
    }
  }

  onMouseMove = (event: MouseEvent) => {
    if (!this.#isMouseDown || !this.canDrag) {
      return
    }

    if (!this.isDragging) {
      const dragThreshold = 5
      this.isDragging =
        this.#orientation === 'horizontal'
          ? Math.abs(event.clientX - this.initialState.clientPos) >
            dragThreshold
          : Math.abs(event.clientY - this.initialState.clientPos) >
            dragThreshold
    }

    if (!this.isDragging) {
      return
    }

    // Prevent default behaviors, e.g. text selection.
    event.preventDefault()
    this.onDragUpdate(
      this.initialState.target as TargetType,
      this.#orientation === 'horizontal' ? event.clientX : event.clientY
    )
  }

  onMouseUp = (event: MouseEvent) => {
    document.removeEventListener('mousemove', this.onMouseMove)
    document.removeEventListener('mouseup', this.onMouseUp)

    this.#isMouseDown = false

    if (this.isDragging && this.canDrag) {
      this.onDragEnd(
        this.initialState.target as TargetType,
        this.#orientation === 'horizontal' ? event.clientX : event.clientY
      )
    } else {
      this.onClick(this.initialState.target as TargetType)
    }
  }
}
