// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import VisibilityTimer from './visibilityTimer'

describe('VisibilityTimer', () => {
  function setVisibilityState(state: DocumentVisibilityState) {
    Object.defineProperty(document, 'visibilityState', {
      configurable: true,
      value: state,
    })
  }

  beforeEach(() => {
    jest.useFakeTimers()
    setVisibilityState('visible')
  })

  afterEach(() => {
    jest.useRealTimers()
  })

  it('does not fire while hidden and resumes after visible', () => {
    const onTimerExpired = jest.fn()
    const timer = new VisibilityTimer(onTimerExpired, 1000)

    timer.startTracking()

    setVisibilityState('hidden')
    document.dispatchEvent(new Event('visibilitychange'))
    jest.advanceTimersByTime(1500)
    expect(onTimerExpired).not.toHaveBeenCalled()

    setVisibilityState('visible')
    document.dispatchEvent(new Event('visibilitychange'))
    jest.advanceTimersByTime(999)
    expect(onTimerExpired).not.toHaveBeenCalled()

    jest.advanceTimersByTime(1)
    expect(onTimerExpired).toHaveBeenCalledTimes(1)

    timer.stopTracking()
  })
})
