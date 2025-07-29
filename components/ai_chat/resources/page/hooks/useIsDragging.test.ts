/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { renderHook, act } from '@testing-library/react'
import { useIsDragging } from './useIsDragging'

describe('useIsDragging', () => {
  const mockSetDragActive = jest.fn()
  const mockSetDragOver = jest.fn()
  const mockClearDragState = jest.fn()

  beforeEach(() => {
    jest.clearAllMocks()
    // Reset DOM event listeners
    document.removeEventListener = jest.fn()
    document.addEventListener = jest.fn()
    window.removeEventListener = jest.fn()
    window.addEventListener = jest.fn()
    jest.useFakeTimers()
  })

  afterEach(() => {
    jest.useRealTimers()
  })

  const renderUseIsDragging = () => {
    return renderHook(() => useIsDragging({
      setDragActive: mockSetDragActive,
      setDragOver: mockSetDragOver,
      clearDragState: mockClearDragState
    }))
  }

  describe('initialization', () => {
    it('sets up document event listeners', () => {
      renderUseIsDragging()

      expect(document.addEventListener)
        .toHaveBeenCalledWith('dragenter', expect.any(Function))
      expect(document.addEventListener)
        .toHaveBeenCalledWith('dragleave', expect.any(Function))
      expect(document.addEventListener)
        .toHaveBeenCalledWith('dragover', expect.any(Function))
      expect(document.addEventListener)
        .toHaveBeenCalledWith('dragend', expect.any(Function))
      expect(window.addEventListener)
        .toHaveBeenCalledWith('blur', expect.any(Function))
      expect(document.addEventListener)
        .toHaveBeenCalledWith('visibilitychange', expect.any(Function))
    })
  })

  describe('document drag events', () => {
    it('activates drag state when files are dragged over document', () => {
      renderUseIsDragging()

      // Get the dragenter handler
      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      act(() => {
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).toHaveBeenCalledWith(true)
      expect(mockSetDragOver).toHaveBeenCalledWith(true)
    })

    it('does not activate for non-file drags', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['text/plain'] }
      } as any

      act(() => {
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).not.toHaveBeenCalled()
      expect(mockSetDragOver).not.toHaveBeenCalled()
    })

    it('clears drag state on dragleave when counter reaches zero', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]
      const dragLeaveHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragleave')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      // Enter drag
      act(() => {
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).toHaveBeenCalledWith(true)

      // Leave drag
      act(() => {
        dragLeaveHandler(mockEvent)
      })

      expect(mockClearDragState).toHaveBeenCalled()
    })

    it('clears drag state on dragend', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]
      const dragEndHandler = (document.addEventListener as jest.Mock).mock.calls
        .find(call => call[0] === 'dragend')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      // Enter drag
      act(() => {
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).toHaveBeenCalledWith(true)

      // End drag
      act(() => {
        dragEndHandler({})
      })

      expect(mockClearDragState).toHaveBeenCalled()
    })

    it('clears drag state on window blur', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]
      const windowBlurHandler = (window.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'blur')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      // Enter drag
      act(() => {
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).toHaveBeenCalledWith(true)

      // Blur window
      act(() => {
        windowBlurHandler({})
      })

      expect(mockClearDragState).toHaveBeenCalled()
    })

    it('clears drag state when page becomes hidden', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]
      const visibilityChangeHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'visibilitychange')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      // Enter drag
      act(() => {
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).toHaveBeenCalledWith(true)

      // Hide page
      Object.defineProperty(document, 'hidden', {
        value: true,
        configurable: true
      })
      act(() => {
        visibilityChangeHandler({})
      })

      expect(mockClearDragState).toHaveBeenCalled()
    })

    it('maintains drag counter for nested elements', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]
      const dragLeaveHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragleave')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      // Enter twice (nested elements)
      act(() => {
        dragEnterHandler(mockEvent)
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).toHaveBeenCalledWith(true)
      expect(mockSetDragOver).toHaveBeenCalledWith(true)

      // Leave once - should not clear yet
      act(() => {
        dragLeaveHandler(mockEvent)
      })

      expect(mockClearDragState).not.toHaveBeenCalled()

      // Leave again - now should clear
      act(() => {
        dragLeaveHandler(mockEvent)
      })

      expect(mockClearDragState).toHaveBeenCalled()
    })
  })

  describe('timeout behavior', () => {
    it('sets timeout to clear drag state after 1 second of inactivity', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      act(() => {
        dragEnterHandler(mockEvent)
      })

      expect(mockSetDragActive).toHaveBeenCalledWith(true)
      expect(mockClearDragState).not.toHaveBeenCalled()

      // Advance timer by 1 second
      act(() => {
        jest.advanceTimersByTime(1000)
      })

      expect(mockClearDragState).toHaveBeenCalled()
    })

    it('resets timeout on dragover activity', () => {
      renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]
      const dragOverHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragover')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      // Start drag
      act(() => {
        dragEnterHandler(mockEvent)
      })

      // Advance timer by 500ms
      act(() => {
        jest.advanceTimersByTime(500)
      })

      // Trigger dragover to reset timeout
      act(() => {
        dragOverHandler(mockEvent)
      })

      // Advance timer by another 500ms (total 1000ms from start)
      act(() => {
        jest.advanceTimersByTime(500)
      })

      // Should not have cleared yet because timeout was reset
      expect(mockClearDragState).not.toHaveBeenCalled()

      // Advance by another 500ms (1000ms from reset)
      act(() => {
        jest.advanceTimersByTime(500)
      })

      // Now should clear
      expect(mockClearDragState).toHaveBeenCalled()
    })
  })

  describe('cleanup', () => {
    it('removes document event listeners on unmount', () => {
      const { unmount } = renderUseIsDragging()

      unmount()

      expect(document.removeEventListener)
        .toHaveBeenCalledWith('dragenter', expect.any(Function))
      expect(document.removeEventListener)
        .toHaveBeenCalledWith('dragleave', expect.any(Function))
      expect(document.removeEventListener)
        .toHaveBeenCalledWith('dragover', expect.any(Function))
      expect(document.removeEventListener)
        .toHaveBeenCalledWith('dragend', expect.any(Function))
      expect(window.removeEventListener)
        .toHaveBeenCalledWith('blur', expect.any(Function))
      expect(document.removeEventListener)
        .toHaveBeenCalledWith('visibilitychange', expect.any(Function))
    })

    it('clears timeout on unmount when timeout exists', () => {
      const { unmount } = renderUseIsDragging()

      const dragEnterHandler = (document.addEventListener as jest.Mock)
        .mock.calls
        .find(call => call[0] === 'dragenter')?.[1]

      const mockEvent = {
        preventDefault: jest.fn(),
        dataTransfer: { types: ['Files'] }
      } as any

      const clearTimeoutSpy = jest.spyOn(global, 'clearTimeout')

      // Create a timeout by triggering dragenter
      act(() => {
        dragEnterHandler(mockEvent)
      })

      unmount()

      expect(clearTimeoutSpy).toHaveBeenCalled()
    })
  })
})