/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { renderHook } from '@testing-library/react'
import { useIframeDragHandling } from './useIframeDragHandling'

// Mock the API
const mockAPI = {
  conversationEntriesFrameObserver: {
    dragStart: {
      addListener: jest.fn()
    },
    removeListener: jest.fn()
  }
}

jest.mock('../api', () => ({
  __esModule: true,
  default: () => mockAPI
}))

describe('useIframeDragHandling', () => {
  const mockSetDragActive = jest.fn()
  const mockSetDragOver = jest.fn()

  beforeEach(() => {
    jest.clearAllMocks()
    // Set up mock return values
    mockAPI.conversationEntriesFrameObserver.dragStart.addListener
      .mockReturnValue('dragStart-listener-id')
  })

  const renderUseIframeDragHandling = () => {
    return renderHook(() => useIframeDragHandling({
      setDragActive: mockSetDragActive,
      setDragOver: mockSetDragOver
    }))
  }

  describe('initialization', () => {
    it('sets up mojom listeners', () => {
      renderUseIframeDragHandling()

      expect(mockAPI.conversationEntriesFrameObserver.dragStart.addListener)
        .toHaveBeenCalled()
    })
  })

  describe('iframe drag events', () => {
    it('activates drag state on iframe dragStart event', () => {
      renderUseIframeDragHandling()

      // Get the dragStart listener callback
      const dragStartCallback = mockAPI.conversationEntriesFrameObserver
        .dragStart.addListener.mock.calls[0][0]

      dragStartCallback()

      expect(mockSetDragActive).toHaveBeenCalledWith(true)
      expect(mockSetDragOver).toHaveBeenCalledWith(true)
    })

  })

  describe('cleanup', () => {
    it('removes mojom listeners on unmount', () => {
      const { unmount } = renderUseIframeDragHandling()

      unmount()

      expect(mockAPI.conversationEntriesFrameObserver.removeListener)
        .toHaveBeenCalledWith('dragStart-listener-id')
    })
  })
})