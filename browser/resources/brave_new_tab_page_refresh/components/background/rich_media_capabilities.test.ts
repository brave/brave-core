/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabPageAdEventType } from '../../state/background_state'

import {
  readIncomingMessage,
  dispatchIncomingMessage,
  RichMediaIncomingMessage,
  RichMediaCapabilities,
} from './rich_media_capabilities'

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
        value: 'click',
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
        value: 'click',
      })
      expect(result).toBeNull()
    })
  })

  describe('valid richMediaEvent messages', () => {
    test('returns correct message for click event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'click',
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kClicked,
      })
    })

    test('returns correct message for interaction event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'interaction',
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kInteraction,
      })
    })

    test('returns correct message for mediaPlay event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'mediaPlay',
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMediaPlay,
      })
    })

    test('returns correct message for media25 event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'media25',
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMedia25,
      })
    })

    test('returns correct message for media100 event', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'media100',
      })
      expect(result).toEqual({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kMedia100,
      })
    })
  })

  describe('invalid richMediaEvent messages', () => {
    test('returns null for richMediaEvent with invalid value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 'invalidEvent',
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
        value: null,
      })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with undefined value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: undefined,
      })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with numeric value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: 123,
      })
      expect(result).toBeNull()
    })

    test('returns null for richMediaEvent with object value', () => {
      const result = readIncomingMessage({
        type: 'richMediaEvent',
        value: { event: 'click' },
      })
      expect(result).toBeNull()
    })
  })
})

describe('dispatchIncomingMessage', () => {
  let mockCapabilities: RichMediaCapabilities

  beforeEach(() => {
    mockCapabilities = {
      notifyAdEvent: jest.fn(),
      openDestinationUrl: jest.fn(),
      openBraveSearch: jest.fn(),
      queryBraveSearchAutocomplete: jest.fn(),
    }
  })

  function dispatch(message: RichMediaIncomingMessage) {
    dispatchIncomingMessage(message, mockCapabilities)
  }

  describe('richMediaEvent message', () => {
    it('should call notifyAdEvent with the event value', () => {
      dispatch({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kInteraction,
      })

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kInteraction,
      )

      expect(mockCapabilities.openDestinationUrl).not.toHaveBeenCalled()
    })

    it('should call openDestinationUrl when event is kClicked', () => {
      dispatch({
        type: 'richMediaEvent',
        value: NewTabPageAdEventType.kClicked,
      })

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kClicked,
      )

      expect(mockCapabilities.openDestinationUrl).toHaveBeenCalled()
    })
  })

  describe('richMediaQueryBraveSearchAutocomplete message', () => {
    it('should call queryBraveSearchAutocomplete with the query value', () => {
      dispatch({
        type: 'richMediaQueryBraveSearchAutocomplete',
        value: 'test query',
      })

      expect(
        mockCapabilities.queryBraveSearchAutocomplete,
      ).toHaveBeenCalledWith('test query')

      expect(mockCapabilities.notifyAdEvent).not.toHaveBeenCalled()
      expect(mockCapabilities.openDestinationUrl).not.toHaveBeenCalled()
      expect(mockCapabilities.openBraveSearch).not.toHaveBeenCalled()
    })
  })

  describe('richMediaOpenBraveSearchWithQuery message', () => {
    it('should call notifyAdEvent with kClicked and openBraveSearch', () => {
      dispatch({
        type: 'richMediaOpenBraveSearchWithQuery',
        value: 'search query',
      })

      expect(mockCapabilities.notifyAdEvent).toHaveBeenCalledWith(
        NewTabPageAdEventType.kClicked,
      )
      expect(mockCapabilities.openBraveSearch).toHaveBeenCalledWith(
        'search query',
      )
    })
  })
})
