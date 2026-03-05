/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { renderHook } from '@testing-library/react'
import { useUntrustedFrameDragHandling } from './useUntrustedFrameDragHandling'

describe('useUntrustedFrameDragHandling', () => {
  let addEventListenerSpy: jest.SpyInstance
  let removeEventListenerSpy: jest.SpyInstance

  beforeEach(() => {
    addEventListenerSpy = jest.spyOn(document, 'addEventListener')
    removeEventListenerSpy = jest.spyOn(document, 'removeEventListener')
  })

  afterEach(() => {
    addEventListenerSpy.mockRestore()
    removeEventListenerSpy.mockRestore()
  })

  describe('initialization', () => {
    it('sets up dragenter event listener on mount', () => {
      const onDragStarted = jest.fn()
      renderHook(() => useUntrustedFrameDragHandling(onDragStarted))

      expect(addEventListenerSpy).toHaveBeenCalledWith(
        'dragenter',
        expect.any(Function),
      )
    })

    it('removes dragenter event listener on unmount', () => {
      const onDragStarted = jest.fn()
      const { unmount } = renderHook(() =>
        useUntrustedFrameDragHandling(onDragStarted),
      )

      unmount()

      expect(removeEventListenerSpy).toHaveBeenCalledWith(
        'dragenter',
        expect.any(Function),
      )
    })
  })

  describe('event handling', () => {
    it('calls onDragStarted when file drag enters', () => {
      const onDragStarted = jest.fn()
      renderHook(() => useUntrustedFrameDragHandling(onDragStarted))

      const dragEnterHandler = addEventListenerSpy.mock.calls.find(
        (call) => call[0] === 'dragenter',
      )?.[1]

      const mockEvent = {
        dataTransfer: { types: ['Files'] },
      } as DragEvent

      dragEnterHandler(mockEvent)

      expect(onDragStarted).toHaveBeenCalledTimes(1)
    })

    it('does not call onDragStarted for non-file drags', () => {
      const onDragStarted = jest.fn()
      renderHook(() => useUntrustedFrameDragHandling(onDragStarted))

      const dragEnterHandler = addEventListenerSpy.mock.calls.find(
        (call) => call[0] === 'dragenter',
      )?.[1]

      const mockEvent = {
        dataTransfer: { types: ['text/plain'] },
      } as DragEvent

      dragEnterHandler(mockEvent)

      expect(onDragStarted).not.toHaveBeenCalled()
    })

    it('calls onDragStarted for each dragenter event', () => {
      const onDragStarted = jest.fn()
      renderHook(() => useUntrustedFrameDragHandling(onDragStarted))

      const dragEnterHandler = addEventListenerSpy.mock.calls.find(
        (call) => call[0] === 'dragenter',
      )?.[1]

      const mockEvent = {
        dataTransfer: { types: ['Files'] },
      } as DragEvent

      dragEnterHandler(mockEvent)
      dragEnterHandler(mockEvent)
      dragEnterHandler(mockEvent)

      expect(onDragStarted).toHaveBeenCalledTimes(3)
    })
  })

  describe('edge cases', () => {
    it('handles missing dataTransfer', () => {
      const onDragStarted = jest.fn()
      renderHook(() => useUntrustedFrameDragHandling(onDragStarted))

      const dragEnterHandler = addEventListenerSpy.mock.calls.find(
        (call) => call[0] === 'dragenter',
      )?.[1]

      const mockEvent = {
        dataTransfer: null,
      } as unknown as DragEvent

      expect(() => {
        dragEnterHandler(mockEvent)
      }).not.toThrow()

      expect(onDragStarted).not.toHaveBeenCalled()
    })

    it('handles missing types array', () => {
      const onDragStarted = jest.fn()
      renderHook(() => useUntrustedFrameDragHandling(onDragStarted))

      const dragEnterHandler = addEventListenerSpy.mock.calls.find(
        (call) => call[0] === 'dragenter',
      )?.[1]

      const mockEvent = {
        dataTransfer: { types: null },
      } as unknown as DragEvent

      expect(() => {
        dragEnterHandler(mockEvent)
      }).not.toThrow()

      expect(onDragStarted).not.toHaveBeenCalled()
    })
  })

  describe('callback updates', () => {
    it('uses updated callback after rerender', () => {
      const onDragStarted1 = jest.fn()
      const onDragStarted2 = jest.fn()

      const { rerender } = renderHook(
        ({ callback }) => useUntrustedFrameDragHandling(callback),
        { initialProps: { callback: onDragStarted1 } },
      )

      rerender({ callback: onDragStarted2 })

      // Get the latest handler (after rerender)
      const dragEnterHandler = addEventListenerSpy.mock.calls
        .filter((call) => call[0] === 'dragenter')
        .pop()?.[1]

      const mockEvent = {
        dataTransfer: { types: ['Files'] },
      } as DragEvent

      dragEnterHandler(mockEvent)

      expect(onDragStarted2).toHaveBeenCalledTimes(1)
    })
  })
})
