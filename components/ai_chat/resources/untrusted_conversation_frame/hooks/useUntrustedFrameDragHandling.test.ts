/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  untrustedFrameDragHandlingSetup,
  registerDragStartCallback,
} from './useUntrustedFrameDragHandling'

// Mock parentUIFrame
const mockParentUIFrame = {
  dragStart: jest.fn(),
}

describe('untrustedFrameDragHandlingSetup', () => {
  beforeEach(() => {
    jest.clearAllMocks()
    // Reset DOM event listeners
    document.addEventListener = jest.fn()
    document.removeEventListener = jest.fn()
  })

  describe('initialization', () => {
    it('sets up dragenter event listener', () => {
      untrustedFrameDragHandlingSetup()

      expect(document.addEventListener).toHaveBeenCalledWith(
        'dragenter',
        expect.any(Function),
      )
    })
  })

  describe('event handling', () => {
    let dragEnterHandler: (e: DragEvent) => void

    beforeEach(() => {
      untrustedFrameDragHandlingSetup()

      // Extract the dragenter handler
      const addEventListenerCalls = (document.addEventListener as jest.Mock)
        .mock.calls
      dragEnterHandler = addEventListenerCalls.find(
        (call) => call[0] === 'dragenter',
      )?.[1]

      // Register the drag start callback
      registerDragStartCallback(mockParentUIFrame as any)
    })

    describe('dragenter handling', () => {
      it('calls dragStart when file drag enters', () => {
        const mockEvent = {
          dataTransfer: { types: ['Files'] },
        } as DragEvent

        dragEnterHandler(mockEvent)

        expect(mockParentUIFrame.dragStart).toHaveBeenCalledTimes(1)
      })

      it('does not call dragStart for non-file drags', () => {
        const mockEvent = {
          dataTransfer: { types: ['text/plain'] },
        } as DragEvent

        dragEnterHandler(mockEvent)

        expect(mockParentUIFrame.dragStart).not.toHaveBeenCalled()
      })

      it('calls dragStart for each dragenter event', () => {
        const mockEvent = {
          dataTransfer: { types: ['Files'] },
        } as DragEvent

        dragEnterHandler(mockEvent)
        dragEnterHandler(mockEvent)
        dragEnterHandler(mockEvent)

        expect(mockParentUIFrame.dragStart).toHaveBeenCalledTimes(3)
      })
    })

    describe('edge cases', () => {
      it('handles missing dataTransfer', () => {
        const mockEvent = {
          dataTransfer: null,
        } as unknown as DragEvent

        expect(() => {
          dragEnterHandler(mockEvent)
        }).not.toThrow()

        expect(mockParentUIFrame.dragStart).not.toHaveBeenCalled()
      })

      it('handles missing types array', () => {
        const mockEvent = {
          dataTransfer: { types: null },
        } as unknown as DragEvent

        expect(() => {
          dragEnterHandler(mockEvent)
        }).not.toThrow()

        expect(mockParentUIFrame.dragStart).not.toHaveBeenCalled()
      })
    })
  })

  describe('before callback registration', () => {
    it('does not throw when drag event occurs before callback is registered', () => {
      // Reset the module to clear any previously registered callback
      jest.resetModules()
      const {
        untrustedFrameDragHandlingSetup: setupFresh,
      } = require('./useUntrustedFrameDragHandling')

      setupFresh()

      const addEventListenerCalls = (document.addEventListener as jest.Mock)
        .mock.calls
      const dragEnterHandler = addEventListenerCalls.find(
        (call: any[]) => call[0] === 'dragenter',
      )?.[1]

      const mockEvent = {
        dataTransfer: { types: ['Files'] },
      } as DragEvent

      // Should not throw even if callback is not registered
      expect(() => {
        dragEnterHandler(mockEvent)
      }).not.toThrow()
    })
  })
})
