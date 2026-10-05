// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, render } from '@testing-library/react'
import {
  ENTRY_UUIDS_ATTRIBUTE,
  HIGHLIGHT_ATTRIBUTE,
  useScrollToEntry,
} from './useScrollToEntry'

function Harness(props: { turns: string[] }) {
  const scrollRef = React.useRef<HTMLDivElement>(null)
  const contentRef = React.useRef<HTMLDivElement>(null)
  const target = useScrollToEntry(scrollRef, contentRef)
  return (
    <div ref={scrollRef}>
      <div
        ref={contentRef}
        data-testid='content'
        data-target={target?.entryUuid ?? ''}
      >
        {props.turns.map((uuids) => (
          <div
            key={uuids}
            data-testid={`turn-${uuids}`}
            {...{ [ENTRY_UUIDS_ATTRIBUTE]: uuids }}
          />
        ))}
      </div>
    </div>
  )
}

describe('useScrollToEntry', () => {
  let scrollIntoView: jest.Mock

  beforeEach(() => {
    jest.useFakeTimers()
    scrollIntoView = jest.fn()
    Element.prototype.scrollIntoView = scrollIntoView
    window.history.replaceState(null, '', '/')
  })

  afterEach(() => {
    jest.useRealTimers()
  })

  function scrolledTurns() {
    return scrollIntoView.mock.contexts.map((turn: HTMLElement) =>
      turn.getAttribute('data-testid'),
    )
  }

  it('does nothing when the fragment names no entry', () => {
    const { getByTestId } = render(<Harness turns={['a', 'b']} />)
    act(() => {
      jest.advanceTimersByTime(2000)
    })
    expect(scrollIntoView).not.toHaveBeenCalled()
    expect(getByTestId('content').dataset.target).toBe('')
  })

  it('scrolls to and highlights the turn showing the entry', () => {
    window.history.replaceState(null, '', '/#entry=b&n=1')
    const { getByTestId } = render(<Harness turns={['a', 'b c', 'd']} />)

    expect(getByTestId('content').dataset.target).toBe('b')
    expect(scrolledTurns()).toEqual(['turn-b c'])
    expect(scrollIntoView).toHaveBeenCalledWith({
      block: 'center',
      behavior: 'instant',
    })
    expect(getByTestId('turn-b c').hasAttribute(HIGHLIGHT_ATTRIBUTE)).toBe(true)
    expect(getByTestId('turn-a').hasAttribute(HIGHLIGHT_ATTRIBUTE)).toBe(false)
  })

  it('keeps the entry in view while the layout settles', () => {
    window.history.replaceState(null, '', '/#entry=a&n=1')
    render(<Harness turns={['a']} />)
    const scrollsAtOpen = scrollIntoView.mock.calls.length

    act(() => {
      jest.advanceTimersByTime(500)
    })
    expect(scrollIntoView.mock.calls.length).toBeGreaterThan(scrollsAtOpen)

    act(() => {
      jest.advanceTimersByTime(2000)
    })
    const scrollsWhenSettled = scrollIntoView.mock.calls.length
    act(() => {
      jest.advanceTimersByTime(2000)
    })
    expect(scrollIntoView.mock.calls.length).toBe(scrollsWhenSettled)
  })

  it('leaves the entry where the user scrolls it', () => {
    window.history.replaceState(null, '', '/#entry=a&n=1')
    const { container } = render(<Harness turns={['a']} />)
    const scrollsAtOpen = scrollIntoView.mock.calls.length

    act(() => {
      container.firstElementChild!.dispatchEvent(new Event('wheel'))
      jest.advanceTimersByTime(2000)
    })
    expect(scrollIntoView.mock.calls.length).toBe(scrollsAtOpen)
  })

  it('waits for the entry to be rendered', async () => {
    window.history.replaceState(null, '', '/#entry=late&n=1')
    const { rerender } = render(<Harness turns={['a']} />)
    expect(scrollIntoView).not.toHaveBeenCalled()

    rerender(<Harness turns={['a', 'late']} />)
    // The mutation observer reports on a microtask.
    await act(async () => {
      await Promise.resolve()
    })
    expect(scrolledTurns()).toContain('turn-late')
  })

  it('scrolls again when the same entry is named again', () => {
    window.history.replaceState(null, '', '/#entry=a&n=1')
    render(<Harness turns={['a']} />)
    act(() => {
      jest.advanceTimersByTime(2000)
    })
    const scrollsBefore = scrollIntoView.mock.calls.length

    act(() => {
      window.history.replaceState(null, '', '/#entry=a&n=2')
      window.dispatchEvent(new HashChangeEvent('hashchange'))
    })
    expect(scrollIntoView.mock.calls.length).toBeGreaterThan(scrollsBefore)
  })
})
