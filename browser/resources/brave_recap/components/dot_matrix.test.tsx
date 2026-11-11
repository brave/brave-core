// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, render } from '@testing-library/react'
import { DotMatrix } from './dot_matrix'

function mockContext() {
  const ctx = {
    clearRect: jest.fn(),
    beginPath: jest.fn(),
    moveTo: jest.fn(),
    arc: jest.fn(),
    fill: jest.fn(),
    setTransform: jest.fn(),
    fillStyle: '',
    globalAlpha: 1,
  }
  jest
    .spyOn(HTMLCanvasElement.prototype, 'getContext')
    .mockReturnValue(ctx as unknown as CanvasRenderingContext2D)
  return ctx
}

function mockReducedMotion(reduced: boolean) {
  jest.spyOn(window, 'matchMedia').mockImplementation(
    (query) =>
      ({
        matches: reduced && query === '(prefers-reduced-motion: reduce)',
        media: query,
        addEventListener: () => {},
        removeEventListener: () => {},
      }) as unknown as MediaQueryList,
  )
}

describe('dot matrix', () => {
  let callbacks: FrameRequestCallback[]

  beforeEach(() => {
    callbacks = []
    jest.spyOn(window, 'requestAnimationFrame').mockImplementation((cb) => {
      callbacks.push(cb)
      return callbacks.length
    })
    jest.spyOn(window, 'cancelAnimationFrame').mockImplementation(() => {})
    for (const [property, value] of [
      ['clientWidth', 240],
      ['clientHeight', 120],
    ] as const) {
      jest
        .spyOn(HTMLCanvasElement.prototype, property, 'get')
        .mockReturnValue(value)
    }
  })
  afterEach(() => jest.restoreAllMocks())

  it('draws nothing when there is no canvas support', () => {
    jest.spyOn(HTMLCanvasElement.prototype, 'getContext').mockReturnValue(null)
    mockReducedMotion(false)
    render(<DotMatrix />)
    expect(callbacks).toHaveLength(0)
  })

  it('animates with one frame requested at a time', () => {
    const ctx = mockContext()
    mockReducedMotion(false)
    render(<DotMatrix />)
    expect(ctx.arc).not.toHaveBeenCalled()

    act(() => callbacks.shift()!(1000))
    const dots = ctx.arc.mock.calls.length
    expect(dots).toBeGreaterThan(0)
    expect(callbacks).toHaveLength(1)

    // Frames closer together than the frame cap are skipped.
    act(() => callbacks.shift()!(1010))
    expect(ctx.arc).toHaveBeenCalledTimes(dots)

    act(() => callbacks.shift()!(1100))
    expect(ctx.arc.mock.calls.length).toBeGreaterThan(dots)
  })

  it('draws a single still frame when motion is reduced', () => {
    const ctx = mockContext()
    mockReducedMotion(true)
    render(<DotMatrix />)
    expect(ctx.arc).toHaveBeenCalled()
    expect(callbacks).toHaveLength(0)
  })

  it('stops animating when removed', () => {
    mockContext()
    mockReducedMotion(false)
    const { unmount } = render(<DotMatrix />)
    unmount()
    expect(window.cancelAnimationFrame).toHaveBeenCalled()
  })
})
