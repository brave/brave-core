/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { selectionEdgeScrollSetup } from './useSelectionEdgeScroll'

// Mock the UntrustedConversationFrameAPI
const mockAPI = {
  parentUIFrame: {
    selectionMouseMove: jest.fn(),
    selectionEnded: jest.fn(),
  },
}

jest.mock('../untrusted_conversation_frame_api', () => ({
  __esModule: true,
  default: {
    getInstance: () => mockAPI,
  },
}))

describe('selectionEdgeScrollSetup', () => {
  let originalGetSelection: typeof globalThis.getSelection
  let mockSelection: { type: string } | null

  beforeEach(() => {
    jest.clearAllMocks()

    // Store originals
    originalGetSelection = globalThis.getSelection

    // Mock getSelection
    mockSelection = null
    Object.defineProperty(globalThis, 'getSelection', {
      value: () => mockSelection,
      configurable: true,
    })

    // Mock document.documentElement.scrollHeight
    Object.defineProperty(document.documentElement, 'scrollHeight', {
      value: 2000,
      configurable: true,
    })

    // Reset DOM event listeners
    document.addEventListener = jest.fn()
    document.removeEventListener = jest.fn()
  })

  afterEach(() => {
    // Restore originals
    Object.defineProperty(globalThis, 'getSelection', {
      value: originalGetSelection,
      configurable: true,
    })
  })

  describe('initialization', () => {
    it('sets up event listeners', () => {
      selectionEdgeScrollSetup()

      expect(document.addEventListener).toHaveBeenCalledWith(
        'mousemove',
        expect.any(Function),
      )
      expect(document.addEventListener).toHaveBeenCalledWith(
        'mouseup',
        expect.any(Function),
      )
      expect(document.addEventListener).toHaveBeenCalledWith(
        'selectionchange',
        expect.any(Function),
      )
    })
  })

  describe('mousemove handling', () => {
    let mouseMoveHandler: (e: MouseEvent) => void

    beforeEach(() => {
      selectionEdgeScrollSetup()

      // Extract the mousemove handler
      const addEventListenerCalls = (document.addEventListener as jest.Mock)
        .mock.calls
      mouseMoveHandler = addEventListenerCalls.find(
        (call) => call[0] === 'mousemove',
      )?.[1]
    })

    it('does not send mouse move when no text is selected', () => {
      mockSelection = { type: 'Caret' }

      const mockEvent = {
        clientY: 100,
        buttons: 1,
      } as MouseEvent

      mouseMoveHandler(mockEvent)

      expect(mockAPI.parentUIFrame.selectionMouseMove).not.toHaveBeenCalled()
    })

    it('does not send mouse move when mouse button is not pressed', () => {
      mockSelection = { type: 'Range' }

      const mockEvent = {
        clientY: 100,
        buttons: 0,
      } as MouseEvent

      mouseMoveHandler(mockEvent)

      expect(mockAPI.parentUIFrame.selectionMouseMove).not.toHaveBeenCalled()
    })

    it('sends mouse position when selecting text with button pressed', () => {
      mockSelection = { type: 'Range' }

      const mockEvent = {
        clientY: 150,
        buttons: 1,
      } as MouseEvent

      mouseMoveHandler(mockEvent)

      expect(mockAPI.parentUIFrame.selectionMouseMove).toHaveBeenCalledWith(
        150,
        2000,
      )
    })

    it('sends selectionEnded when selection changes to Caret', () => {
      mockSelection = { type: 'Range' }

      // First, start selecting
      const mockEvent = {
        clientY: 150,
        buttons: 1,
      } as MouseEvent

      mouseMoveHandler(mockEvent)
      expect(mockAPI.parentUIFrame.selectionMouseMove).toHaveBeenCalled()

      // Then stop selecting
      mockSelection = { type: 'Caret' }
      mouseMoveHandler(mockEvent)

      expect(mockAPI.parentUIFrame.selectionEnded).toHaveBeenCalled()
    })
  })

  describe('mouseup handling', () => {
    let mouseMoveHandler: (e: MouseEvent) => void
    let mouseUpHandler: () => void

    beforeEach(() => {
      selectionEdgeScrollSetup()

      const addEventListenerCalls = (document.addEventListener as jest.Mock)
        .mock.calls
      mouseMoveHandler = addEventListenerCalls.find(
        (call) => call[0] === 'mousemove',
      )?.[1]
      mouseUpHandler = addEventListenerCalls.find(
        (call) => call[0] === 'mouseup',
      )?.[1]
    })

    it('sends selectionEnded on mouseup when was selecting', () => {
      mockSelection = { type: 'Range' }

      // Start selecting
      const mockEvent = {
        clientY: 150,
        buttons: 1,
      } as MouseEvent

      mouseMoveHandler(mockEvent)

      // Trigger mouseup
      mouseUpHandler()

      expect(mockAPI.parentUIFrame.selectionEnded).toHaveBeenCalled()
    })

    it('does not send selectionEnded on mouseup when was not selecting', () => {
      mouseUpHandler()

      expect(mockAPI.parentUIFrame.selectionEnded).not.toHaveBeenCalled()
    })
  })

  describe('selectionchange handling', () => {
    let mouseMoveHandler: (e: MouseEvent) => void
    let selectionChangeHandler: () => void

    beforeEach(() => {
      selectionEdgeScrollSetup()

      const addEventListenerCalls = (document.addEventListener as jest.Mock)
        .mock.calls
      mouseMoveHandler = addEventListenerCalls.find(
        (call) => call[0] === 'mousemove',
      )?.[1]
      selectionChangeHandler = addEventListenerCalls.find(
        (call) => call[0] === 'selectionchange',
      )?.[1]
    })

    it('sends selectionEnded when selection is cleared', () => {
      mockSelection = { type: 'Range' }

      // Start selecting
      const mockEvent = {
        clientY: 150,
        buttons: 1,
      } as MouseEvent

      mouseMoveHandler(mockEvent)

      // Selection changes to Caret (no range)
      mockSelection = { type: 'Caret' }
      selectionChangeHandler()

      expect(mockAPI.parentUIFrame.selectionEnded).toHaveBeenCalled()
    })
  })
})
