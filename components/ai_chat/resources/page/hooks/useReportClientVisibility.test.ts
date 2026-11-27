// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react'
import useReportClientVisibility from './useReportClientVisibility'

let visibilityState: DocumentVisibilityState = 'visible'

function setVisibility(state: DocumentVisibilityState) {
  visibilityState = state
  act(() => {
    document.dispatchEvent(new Event('visibilitychange'))
  })
}

beforeAll(() => {
  Object.defineProperty(document, 'visibilityState', {
    configurable: true,
    get: () => visibilityState,
  })
})

afterAll(() => {
  delete (document as { visibilityState?: unknown }).visibilityState
})

beforeEach(() => {
  visibilityState = 'visible'
})

function render(setClientVisible: (visible: boolean) => void, shown = true) {
  return renderHook(
    ({ setter, isShown }) => useReportClientVisibility(setter, isShown),
    { initialProps: { setter: setClientVisible, isShown: shown } },
  )
}

describe('useReportClientVisibility', () => {
  it('reports visible when the page shows the conversation', () => {
    const setClientVisible = jest.fn()
    render(setClientVisible)
    expect(setClientVisible.mock.calls).toEqual([[true]])
  })

  it('reports not visible when the page is hidden', () => {
    visibilityState = 'hidden'
    const setClientVisible = jest.fn()
    render(setClientVisible)
    expect(setClientVisible.mock.calls).toEqual([[false]])
  })

  it('reports not visible while the conversation is covered', () => {
    const setClientVisible = jest.fn()
    const { rerender } = render(setClientVisible)
    rerender({ setter: setClientVisible, isShown: false })
    setVisibility('hidden')
    setVisibility('visible')
    rerender({ setter: setClientVisible, isShown: true })
    expect(setClientVisible.mock.calls).toEqual([
      [true],
      [false],
      [false],
      [false],
      [true],
    ])
  })

  it('reports every page visibility change until unmounted', () => {
    const setClientVisible = jest.fn()
    const { unmount } = render(setClientVisible)
    setVisibility('hidden')
    setVisibility('visible')
    unmount()
    setVisibility('hidden')
    expect(setClientVisible.mock.calls).toEqual([[true], [false], [true]])
  })

  it('reports to the new binding when the conversation changes', () => {
    const oldBinding = jest.fn()
    const newBinding = jest.fn()
    const { rerender } = render(oldBinding)
    rerender({ setter: newBinding, isShown: true })
    setVisibility('hidden')
    expect(oldBinding.mock.calls).toEqual([[true]])
    expect(newBinding.mock.calls).toEqual([[true], [false]])
  })
})
