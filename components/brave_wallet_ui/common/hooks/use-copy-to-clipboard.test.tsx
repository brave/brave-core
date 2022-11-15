// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { renderHook, act } from '@testing-library/react-hooks'
import {
  useCopyToClipboard,
  useTemporaryCopyToClipboard
} from './use-copy-to-clipboard'

describe('useCopyToClipboard Hook', () => {
  it('should have false as initial state', () => {
    const { result } = renderHook(() => useCopyToClipboard())
    expect(result.current.isCopied).toBe(false)
  })

  it('should change copied to true when copyText is called', async () => {
    const { result } = renderHook(() => useCopyToClipboard())
    await act(async () => {
      await result.current.copyToClipboard('some text')
    })
    expect(result.current.isCopied).toBe(true)
  })
})

describe('useTemporaryCopyToClipboard Hook', () => {
  it('should not change copied to false until after timeout', async () => {
    jest.useFakeTimers()
    jest.clearAllTimers()

    const timeoutTime = 5000 // 5 seconds

    const { result } = renderHook(() => useTemporaryCopyToClipboard(timeoutTime))

    await act(async () => {
      await result.current.temporaryCopyToClipboard('some text')
      expect(result.current.isCopied).toBe(true)
    })

    await act(async () => {
      jest.advanceTimersByTime(timeoutTime - 100)
    })
    expect(result.current.isCopied).toBe(true)

    await act(async () => {
      jest.advanceTimersByTime(100)
    })
    expect(result.current.isCopied).toBe(false)
  })
})
