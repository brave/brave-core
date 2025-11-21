// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { renderHook, act } from '@testing-library/react'
import { createMemoryHistory } from 'history'
import { Router } from 'react-router-dom'

// Utils
import {
  createMockStore,
  renderHookOptionsWithMockStore,
} from '../../utils/test-utils'

// Hooks
import { useRoute } from './use_route'

// Mocks
jest.mock('../../utils/routes-utils', () => ({
  openWalletRouteTab: jest.fn(),
}))

import { openWalletRouteTab } from '../../utils/routes-utils'

const mockOpenWalletRouteTab = openWalletRouteTab as jest.MockedFunction<
  typeof openWalletRouteTab
>

describe('useRoute hook', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('should call openWalletRouteTab when isPanel is true', () => {
    // setup
    const store = createMockStore({
      uiStateOverride: {
        isPanel: true,
      },
    })
    const history = createMemoryHistory()
    const storeWrapper = renderHookOptionsWithMockStore(store).wrapper
    const renderOptions = {
      wrapper: ({ children }: { children?: React.ReactNode }) => (
        <Router history={history}>{storeWrapper({ children })}</Router>
      ),
    }

    const { result } = renderHook(() => useRoute(), renderOptions)

    // execute
    act(() => {
      result.current.openOrPushRoute('/test-route')
    })

    // assert
    expect(mockOpenWalletRouteTab).toHaveBeenCalledWith('/test-route')
    expect(mockOpenWalletRouteTab).toHaveBeenCalledTimes(1)
    expect(history.location.pathname).toBe('/')
  })

  it('should call history.push when isPanel is false', () => {
    // setup
    const store = createMockStore({
      uiStateOverride: {
        isPanel: false,
      },
    })
    const history = createMemoryHistory()
    const storeWrapper = renderHookOptionsWithMockStore(store).wrapper
    const renderOptions = {
      wrapper: ({ children }: { children?: React.ReactNode }) => (
        <Router history={history}>{storeWrapper({ children })}</Router>
      ),
    }

    const { result } = renderHook(() => useRoute(), renderOptions)

    // execute
    act(() => {
      result.current.openOrPushRoute('/test-route')
    })

    // assert
    expect(mockOpenWalletRouteTab).not.toHaveBeenCalled()
    expect(history.location.pathname).toBe('/test-route')
  })
})
