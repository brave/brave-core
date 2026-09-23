// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { renderHook, act } from '@testing-library/react'
import {
  useCopyToClipboard,
  useTemporaryCopyToClipboard,
} from './use-copy-to-clipboard'

// Utils
import {
  createMockStore,
  renderHookOptionsWithMockStore,
} from '../../utils/test-utils'

describe('useCopyToClipboard Hook', () => {
  it('should have false as initial state', () => {
    const store = createMockStore({})
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(() => useCopyToClipboard(), renderOptions)
    expect(result.current.isCopied).toBe(false)
  })

  it('should change copied to true when copyText is called', async () => {
    const store = createMockStore({})
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(() => useCopyToClipboard(), renderOptions)
    await act(async () => {
      await result.current.copyToClipboard('some text')
    })
    expect(result.current.isCopied).toBe(true)
  })

  it('should copy to clipboard confidentially', async () => {
    const store = createMockStore({})
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () => useCopyToClipboard(undefined, true),
      renderOptions,
    )
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
    const store = createMockStore({})
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () => useTemporaryCopyToClipboard(timeoutTime),
      renderOptions,
    )

    await act(async () => {
      await result.current.temporaryCopyToClipboard('some text')
    })
    expect(result.current.isCopied).toBe(true)

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
