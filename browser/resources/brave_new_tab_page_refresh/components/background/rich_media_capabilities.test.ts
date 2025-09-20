/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabPageAdEventType } from '../../state/background_state'

import {
  readIncomingMessage,
  handleIncomingMessage,
  RichMediaIncomingMessage,
  RichMediaCapabilities } from './rich_media_capabilities'

describe('readIncomingMessage', () => {
  describe('null and undefined data', () => {
    test('returns null when data is null', () => {
      const result = readIncomingMessage(null)
      expect(result).toBeNull()
    })

    test('returns null when data is undefined', () => {
      const result = readIncomingMessage(undefined)
      expect(result).toBeNull()
    })

    test('returns null when data is empty object', () => {
      const result = readIncomingMessage({})
      expect(result).toBeNull()
    })
  })

  describe('invalid message types', () => {
    test('returns null for unknown message type', () => {
      const result = readIncomingMessage({
        type: 'unknownType',
        value: 'click'
      })
      expect(result).toBeNull()
    })

    test('returns null when type is missing', () => {
      const result = readIncomingMessage({ value: 'click' })
      expect(result).toBeNull()
    })

    test('returns null when type is not a string', () => {
      const result = readIncomingMessage({
        type: 123,
        value: 'click'
      })
      expect(result).toBeNull()
    })
  })

  describe('valid richMediaEvent messages', () => {
    test('returns correct message for click event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'click'
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kClicked
      })
    })

    test('returns correct message for interaction event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'interaction'
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kInteraction
      })
    })

    test('returns correct message for mediaPlay event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'mediaPlay'
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMediaPlay
      })
    })

    test('returns correct message for media25 event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'media25'
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMedia25
      })
    })

    test('returns correct message for media100 event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'media100'
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMedia100
      })
    })
  })

  describe('invalid richMediaEvent messages', () => {
    test('returns null for richMediaEvent with invalid value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'invalidEvent'
      })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with missing value', () => {
      const result = readIncomingMessage({ type: 'richMediaEvent' })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with null value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: null
      })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with undefined value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: undefined
      })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with numeric value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 123
      })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with object value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: { event: 'click' }
      })
      expect(result).toBeNull()
    })
  })
})

describe('handleIncomingMessage', () => {
  let mockCapabilities: RichMediaCapabilities

  beforeEach(() => {
    mockCapabilities = {
      notifyAdEvent: jest.fn(),
      openDestinationUrl: jest.fn()
    }
  })

  describe('richMediaEvent message type', () => {
    it('should call notifyAdEvent with the event value', () => {
      const message: RichMediaIncomingMessage = {
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kInteraction
      }

      handleIncomingMessage(message, mockCapabilities)

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kInteraction
      )
    })

    it('should call openDestinationUrl when event is kClicked', () => {
      const message: RichMediaIncomingMessage = {
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kClicked
      }

      handleIncomingMessage(message, mockCapabilities)

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kClicked
      )
      expect(mockCapabilities.openDestinationUrl).toHaveBeenCalled()
    })

    it('should not call openDestinationUrl for kInteraction event', () => {
      const message: RichMediaIncomingMessage = {
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kInteraction
      }

      handleIncomingMessage(message, mockCapabilities)

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kInteraction
      )
      expect(mockCapabilities.openDestinationUrl).not.toHaveBeenCalled()
    })

    it('should not call openDestinationUrl for kMediaPlay event', () => {
      const message: RichMediaIncomingMessage = {
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMediaPlay
      }

      handleIncomingMessage(message, mockCapabilities)

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kMediaPlay
      )
      expect(mockCapabilities.openDestinationUrl).not.toHaveBeenCalled()
    })

    it('should not call openDestinationUrl for kMedia25 event', () => {
      const message: RichMediaIncomingMessage = {
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMedia25
      }

      handleIncomingMessage(message, mockCapabilities)

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kMedia25
      )
      expect(mockCapabilities.openDestinationUrl).not.toHaveBeenCalled()
    })

    it('should not call openDestinationUrl for kMedia100 event', () => {
      const message: RichMediaIncomingMessage = {
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMedia100
      }

      handleIncomingMessage(message, mockCapabilities)

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kMedia100
      )
      expect(mockCapabilities.openDestinationUrl).not.toHaveBeenCalled()
    })
  })
})
