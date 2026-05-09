// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { renderHook } from '@testing-library/react'
import { useVisitDisplayAdClickHandler } from './useReadArticleClickHandler'

describe('useVisitDisplayAdClickHandler', () => {
  const makePayload = (url: string) => ({
    ad: {
      targetUrl: { url },
    },
  }) as any

  const makeEvent = (overrides?: Partial<React.MouseEvent>) => ({
    preventDefault: jest.fn(),
    ctrlKey: false,
    metaKey: false,
    ...overrides,
  }) as React.MouseEvent

  it('dispatches for https ad target URLs', () => {
    const action = jest.fn()
    const payload = makePayload('https://brave.com')
    const { result } = renderHook(() => useVisitDisplayAdClickHandler(action, payload))

    const event = makeEvent({ ctrlKey: true })
    result.current(event)

    expect(event.preventDefault).toHaveBeenCalledTimes(1)
    expect(action).toHaveBeenCalledTimes(1)
    expect(action).toHaveBeenCalledWith({
      ...payload,
      openInNewTab: true,
    })
  })

  it.each([
    'http://example.com',
    'chrome://settings',
    'brave://settings',
    'file:///tmp/example.txt',
    'javascript:alert(1)',
  ])('rejects non-https ad target URL: %s', (url) => {
    const action = jest.fn()
    const payload = makePayload(url)
    const { result } = renderHook(() => useVisitDisplayAdClickHandler(action, payload))

    const event = makeEvent()
    expect(() => result.current(event)).toThrow('Unsupported scheme')
    expect(action).not.toHaveBeenCalled()
  })
})
